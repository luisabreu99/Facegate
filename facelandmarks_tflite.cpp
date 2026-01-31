#include "facelandmarks_tflite.h"

// aqui entra o código que TU JÁ TENS do FaceMesh.cpp

struct FaceLandmarksTFLite::Impl
{
    // Interpreter
    // Modelo
};

FaceLandmarksTFLite::FaceLandmarksTFLite()
{
    // carregar modelo
    // criar interpreter
}

FaceLandmarksTFLite::~FaceLandmarksTFLite() = default;

std::vector<cv::Point3f> FaceLandmarksTFLite::detect(const cv::Mat& face)
{
    // 1. resize face → 192x192
    // 2. normalizar
    // 3. invoke()
    // 4. devolver vector<cv::Point3f> (468 pontos)
}
