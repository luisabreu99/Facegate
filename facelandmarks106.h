#pragma once
#include <opencv2/dnn.hpp>
#include <vector>

class FaceLandmarks106
{
public:
    FaceLandmarks106(const std::string& modelPath);
    std::vector<cv::Point3f> detect(const cv::Mat& face);

private:
    cv::dnn::Net net;
    const int inputSize = 192; // tamanho correto para 2d106det
};
