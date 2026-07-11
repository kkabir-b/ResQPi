#include <iostream>
#include <opencv2/opencv.hpp>

int main() {
    cv:: VideoCapture cap(0);

    cv:: Mat frame;

    while (true){
        cap >> frame;
        cv::imshow("Webcam feed",frame);


        if (cv::waitKey(30) == 27){
            break;
        }

    }

    return 0;
}
