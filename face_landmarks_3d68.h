#pragma once
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>

class FaceLandmarks3D68 {
public:
    FaceLandmarks3D68(const std::string& modelPath);

    std::vector<cv::Point3f> detect(const cv::Mat& face);

private:
    cv::dnn::Net net;
    const int inputSize = 112;
};
