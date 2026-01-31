#include "facelandmarks106.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/dnn.hpp>
#include <QDebug>
FaceLandmarks106::FaceLandmarks106(const std::string& modelPath)
{
    net = cv::dnn::readNetFromONNX(modelPath);
    if (net.empty())
        throw std::runtime_error("Erro a carregar 2d106det.onnx");
}

std::vector<cv::Point3f> FaceLandmarks106::detect(const cv::Mat& face)
{
    cv::Mat input;
    cv::resize(face, input, cv::Size(inputSize, inputSize));
    cv::cvtColor(input, input, cv::COLOR_BGR2RGB);
    input.convertTo(input, CV_32F, 1.0 / 255.0);

    cv::Mat blob = cv::dnn::blobFromImage(input);
    net.setInput(blob);

    cv::Mat out = net.forward(); // [1, 212]
    const float* data = out.ptr<float>();

    std::vector<cv::Point3f> points;
    points.reserve(106);

    for (int i = 0; i < 106; ++i)
    {
        float x_norm = data[i * 2 + 0];   // [-1..1]
        float y_norm = data[i * 2 + 1];   // [-1..1]

        float x = (x_norm * 0.5f + 0.5f) * face.cols;
        float y = (y_norm * 0.5f + 0.5f) * face.rows;

        points.emplace_back(x, y, 0.0f);
    }

    return points;
}
