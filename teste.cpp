#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    cv::VideoCapture cap(0, cv::CAP_DSHOW);
    if (!cap.isOpened()) {
        std::cout << "Camera nao abriu\n";
        return -1;
    }

    cv::Mat f;
    while (true) {
        cap >> f;
        if (f.empty()) break;
        cv::imshow("Teste", f);
        if (cv::waitKey(1) == 27) break;
    }
}
