#include "Inferrer.hpp"
#include <opencv2/opencv.hpp>
#include <tuple>
#include <string>
#include <utility>
#include <vector>
#include <onnxruntime_cxx_api.h>
#include <librdkafka/rdkafkacpp.h>
#include <memory>
#include <string>




Inferrer::Inferrer(const char* path):env(ORT_LOGGING_LEVEL_WARNING,"InferrerInstance"),session(nullptr){
    Ort::SessionOptions session_options;
    session_options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    session_options.EnableCpuMemArena();
    session_options.SetIntraOpNumThreads(2);
    session_options.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);

    session = Ort::Session(env,path,session_options);

    std::string errstr;
    std::unique_ptr<RdKafka::Conf> conf(RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL));
    
    
    if (conf->set("bootstrap.servers", "localhost:9092", errstr) != RdKafka::Conf::CONF_OK) {
        std::cerr << "[Kafka Config Error] " << errstr << std::endl;
    } else {
        kafka_producer.reset(RdKafka::Producer::create(conf.get(), errstr));
        if (!kafka_producer) {
            std::cerr << "[Kafka Create Error] " << errstr << std::endl;
        }
    }
    
    kafka_topic = "detection-events"; 
}

std::tuple<cv::Mat, float, std::pair<float, float>>
Inferrer::letterbox(const cv::Mat& img,
                    int newShape,
                    const cv::Scalar& color)
{
    int h = img.rows;
    int w = img.cols;

    float r = std::min(
        static_cast<float>(newShape) / h,
        static_cast<float>(newShape) / w);

    int newW = static_cast<int>(std::round(w * r));
    int newH = static_cast<int>(std::round(h * r));

    float dw = (newShape - newW) / 2.0f;
    float dh = (newShape - newH) / 2.0f;

    cv::Mat resized;

    if (w != newW || h != newH)
        cv::resize(img, resized, cv::Size(newW, newH), 0, 0, cv::INTER_LINEAR);
    else
        resized = img.clone();

    int top = static_cast<int>(std::round(dh - 0.1f));
    int bottom = static_cast<int>(std::round(dh + 0.1f));
    int left = static_cast<int>(std::round(dw - 0.1f));
    int right = static_cast<int>(std::round(dw + 0.1f));

    cv::Mat padded;

    cv::copyMakeBorder(
        resized,
        padded,
        top,
        bottom,
        left,
        right,
        cv::BORDER_CONSTANT,
        color);

    return {padded, r, {dw, dh}};
}

std::tuple<std::vector<float>, float, std::pair<float, float>>
Inferrer::preProcess(const cv::Mat& imgBGR, int imgSize)
{
    cv::Mat imgRGB;
    cv::cvtColor(imgBGR, imgRGB, cv::COLOR_BGR2RGB);

    auto [padded, ratio, pad] = letterbox(imgRGB, imgSize);

    cv::Mat floatImg;
    padded.convertTo(floatImg, CV_32FC3,1.0f/255.0f);

    std::vector<float> inputTensor(3 * imgSize * imgSize);

    const int channelSize = imgSize * imgSize;

    for (int y = 0; y < imgSize; ++y)
    {
        for (int x = 0; x < imgSize; ++x)
        {
            const cv::Vec3f& pixel = floatImg.at<cv::Vec3f>(y, x);

            inputTensor[0 * channelSize + y * imgSize + x] = pixel[0]; 
            inputTensor[1 * channelSize + y * imgSize + x] = pixel[1]; 
            inputTensor[2 * channelSize + y * imgSize + x] = pixel[2]; 
        }
    }

    return {inputTensor, ratio, pad};
}




bool Inferrer::postProcess(const float* output, size_t num_detections, float confidence_thresh) {

    for (size_t i = 0; i < num_detections; ++i) {
        const float* det = output + (i * 6);
        if (det[4] >= confidence_thresh && (int)det[5] == 4) {
            return true;
        }
    }
    return false;
}

void Inferrer::runInfer(const cv::Mat& inputImg)
{
    int imgSize = 640;
    
    auto [inputTensor, ratio, pad] = preProcess(inputImg, imgSize);

    std::vector<int64_t> inputShape = {1, 3, imgSize, imgSize};
    auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

    Ort::Value inputOrtTensor = Ort::Value::CreateTensor<float>(
        memoryInfo, 
        inputTensor.data(), 
        inputTensor.size(), 
        inputShape.data(), 
        inputShape.size()
    );


    Ort::AllocatorWithDefaultOptions allocator;
    Ort::AllocatedStringPtr inputNamePtr = session.GetInputNameAllocated(0, allocator);
    Ort::AllocatedStringPtr outputNamePtr = session.GetOutputNameAllocated(0, allocator);
    
    std::vector<const char*> inputNames = {inputNamePtr.get()};
    std::vector<const char*> outputNames = {outputNamePtr.get()};

    std::vector<Ort::Value> outputTensors = session.Run(
        Ort::RunOptions{nullptr},
        inputNames.data(),
        &inputOrtTensor,
        1,
        outputNames.data(),
        1
    );

    float* outputData = outputTensors[0].GetTensorMutableData<float>();
    
    auto outputTypeInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
    std::vector<int64_t> outputShape = outputTypeInfo.GetShape();
    
    size_t num_detections = outputShape[1]; 

    bool targetDetected = postProcess(outputData, num_detections, 0.75f);

    if (targetDetected) {
        std::cout << "[Inferrer] Target gesture detected!" << std::endl;

        if (kafka_producer) {
            std::string payload = "\"Location A\"";

            RdKafka::ErrorCode resp = kafka_producer->produce(
                kafka_topic,
                RdKafka::Topic::PARTITION_UA,
                RdKafka::Producer::RK_MSG_COPY,
                const_cast<char*>(payload.c_str()), 
                payload.size(),
                nullptr, // key
                0,       // key length
                0,       // timestamp
                nullptr  // msg_opaque
            );

            if (resp == RdKafka::ERR_NO_ERROR) {
                std::cout << "[Kafka] Successfully published: " << payload << std::endl;
            } else {
                std::cerr << "[Kafka Error] Failed to send message: " << RdKafka::err2str(resp) << std::endl;
            }
            
            
            kafka_producer->poll(0);}
    } 
    else {
        std::cout << "Call gesture not found." << std::endl;
    }
}