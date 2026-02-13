#ifndef FACEEMBEDDER_H
#define FACEEMBEDDER_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <vector>
#include <QElapsedTimer>
class FaceEmbedder : public QObject
{
    Q_OBJECT
public:
    explicit FaceEmbedder(QObject *parent = nullptr);

public slots:
    void extractEmbedding(const cv::Mat &face112);

signals:
    void embeddingReady(const std::vector<float> &embedding);

private:
    cv::dnn::Net net;
    bool initialized = false;
    QElapsedTimer embedTimer;
};

#endif
