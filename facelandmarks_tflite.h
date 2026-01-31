#pragma once
#include "facelandmarks.h"
#include <memory>

class FaceLandmarksTFLite : public IFaceLandmarks
{
public:
    FaceLandmarksTFLite();
    ~FaceLandmarksTFLite();

    std::vector<cv::Point3f> detect(const cv::Mat& face) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
