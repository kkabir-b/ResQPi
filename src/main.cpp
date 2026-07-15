#include "inferrer/Inferrer.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    try {
        std::string modelPath = "models/YOLOv10n_gestures.onnx"; 
        std::cout << "Loading model: " << modelPath << "..." << std::endl;
        Inferrer yolo(modelPath.c_str());
        std::cout << "Model loaded successfully!" << std::endl;

        cv::VideoCapture cap(0);
        if (!cap.isOpened()) {
            std::cerr << "Error: Could not open the webcam." << std::endl;
            return -1;
        }

        cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
        cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

        cv::Mat frame;
        std::cout << "Starting headless inference loop. Press Ctrl+C in the terminal to stop." << std::endl;

        while (true) {
            cap >> frame;
            if (frame.empty()) {
                std::cerr << "Error: Captured an empty frame." << std::endl;
                break;
            }


            yolo.runInfer(frame);
            
        }

        cap.release();

    } catch (const Ort::Exception& e) {
        std::cerr << "ONNX Runtime Error: " << e.what() << std::endl;
        return -1;
    } catch (const std::exception& e) {
        std::cerr << "Standard Exception Error: " << e.what() << std::endl;
        return -1;
    }

    return 0;
}