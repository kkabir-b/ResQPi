#include <iostream>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h> // Include the ONNX Runtime C++ API Header

int main() {
    // 1. Verify ONNX Runtime works by initializing its Environment
    try {
        Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "OnnxTest");
        Ort::SessionOptions session_options;
        std::cout << "Successfully initialized ONNX Runtime!" << std::endl;
    } 
    catch (const std::exception& e) {
        std::cerr << "ONNX Runtime initialization failed: " << e.what() << std::endl;
        return -1;
    }

    // 2. Open the Webcam feed
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Error: Could not open the webcam." << std::endl;
        return -1;
    }

    cv::Mat frame;
    std::cout << "Starting webcam feed. Press 'ESC' to exit." << std::endl;

    while (true) {
        cap >> frame;
        
        // Check if the frame is empty
        if (frame.empty()) {
            std::cerr << "Blank frame grabbed." << std::endl;
            break;
        }

        cv::imshow("Webcam feed", frame);

        // Break the loop if ESC (ASCII 27) is pressed
        if (cv::waitKey(30) == 27) {
            break;
        }
    }

    return 0;
}
