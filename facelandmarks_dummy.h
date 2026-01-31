#pragma once
#include "facelandmarks.h"

class FaceLandmarksDummy : public IFaceLandmarks
{
public:
    std::vector<cv::Point3f> detect(const cv::Mat&) override
    {
        return {};
    }
};
