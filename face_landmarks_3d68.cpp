#include "face_landmarks_3d68.h"


FaceLandmarks3D68::FaceLandmarks3D68(const std::string& modelPath)
{
    net = cv::dnn::readNetFromONNX(modelPath);
    if (net.empty())
        throw std::runtime_error("Erro ao carregar 1k3d68.onnx");
}

std::vector<cv::Point3f> FaceLandmarks3D68::detect(const cv::Mat& face)
{
    cv::Mat input;
    cv::resize(face, input, cv::Size(inputSize, inputSize));
    cv::cvtColor(input, input, cv::COLOR_BGR2RGB);

    // InsightFace normalização correta
    input.convertTo(input, CV_32F);
    input = (input - 127.5f) / 128.0f;

    cv::Mat blob = cv::dnn::blobFromImage(input);
    net.setInput(blob);

    cv::Mat out = net.forward();   // shape: [1, 204] (68 * 3)

    const float* data = out.ptr<float>();

    std::vector<cv::Point3f> points;
    points.reserve(68);

    for (int i = 0; i < 68; ++i)
    {
        float x = data[i * 3 + 0] * face.cols / inputSize;
        float y = data[i * 3 + 1] * face.rows / inputSize;
        float z = data[i * 3 + 2];

        points.emplace_back(x, y, z);
    }

    return points;
}
