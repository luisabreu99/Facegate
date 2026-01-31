#pragma once
#include <opencv2/opencv.hpp>
#include <vector>

class IFaceLandmarks
{
public:
    virtual std::vector<cv::Point3f> detect(const cv::Mat& face) = 0;
    virtual ~IFaceLandmarks() = default;
};
