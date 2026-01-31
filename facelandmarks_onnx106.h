#pragma once
#include "facelandmarks.h"
#include <opencv2/dnn.hpp>

class FaceLandmarksONNX106 : public IFaceLandmarks
{
public:
    FaceLandmarksONNX106(const std::string& modelPath);

    std::vector<cv::Point3f> detect(const cv::Mat& face) override;

private:
    cv::dnn::Net net;
    int inputSize = 192; // depende do modelo (ajustamos depois)
};
