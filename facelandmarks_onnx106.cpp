#include "facelandmarks_onnx106.h"

FaceLandmarksONNX106::FaceLandmarksONNX106(const std::string& modelPath)
{
    net = cv::dnn::readNetFromONNX(modelPath);
    if (net.empty())
        throw std::runtime_error("Erro a carregar landmark_106.onnx");
}

std::vector<cv::Point3f> FaceLandmarksONNX106::detect(const cv::Mat& face)
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

    const float scaleX = face.cols * 0.9f;
    const float scaleY = face.rows * 0.9f;
    const float offsetX = face.cols * 0.05f;
    const float offsetY = face.rows * 0.05f;

    for (int i = 0; i < 106; ++i)
    {
        float x = data[i * 2 + 0];   // [-1,1]
        float y = data[i * 2 + 1];

        float px = (x + 1.0f) * 0.5f * scaleX + offsetX;
        float py = (y + 1.0f) * 0.5f * scaleY + offsetY;

        points.emplace_back(px, py, 0.0f);
    }

    return points;
}
