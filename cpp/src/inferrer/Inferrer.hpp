#pragma once

#include <opencv2/opencv.hpp>
#include <tuple>
#include <string>
#include <utility>
#include <vector>
#include <onnxruntime_cxx_api.h>

class Inferrer {
private:
    Ort::Env env;
    Ort::Session session;
    
    std::tuple<cv::Mat, float, std::pair<float,float>> letterbox(const cv::Mat& img, int newShape = 640, const cv::Scalar& color = cv::Scalar(114, 114, 114));
    std::tuple<std::vector<float>, float, std::pair<float,float>> preProcess(const cv::Mat& imgBGR, int imgSize = 640);
    bool postProcess(const float* output, size_t num_detections, float confidence_thresh = 0.1f);

public:
    Inferrer(const char* path); 
    void runInfer(const cv::Mat& inputImg);
};