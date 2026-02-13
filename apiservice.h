#ifndef APISERVICE_H
#define APISERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <vector>

class ApiService : public QObject
{
    Q_OBJECT
public:
    explicit ApiService(QObject *parent = nullptr);

    void sendColaborador(const QString &nome,
                         const QString &apelido,
                         const QString &dataNasc,
                         const std::vector<float> &embedding);

    void sendRegisto(int colaboradorId,
                     const QString &tipo);

private:
    QNetworkAccessManager manager;
    QString baseUrl = "http://192.168.0.100:8000/api/";
};

#endif
