#include "apiservice.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QDebug>
#include <QJsonArray>
#include <QNetworkReply>

ApiService::ApiService(QObject *parent)
    : QObject(parent)
{
}


void ApiService::sendColaborador(const QString &nome,
                                 const QString &apelido,
                                 const QString &dataNasc,
                                 const std::vector<float> &embedding)
{
    QUrl url(baseUrl + "colaboradores");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // converter embedding → JSON array
    QJsonArray embArray;
    for (float v : embedding)
        embArray.append(v);

    QJsonObject json;
    json["nome"] = nome;
    json["apelido"] = apelido;
    json["data_nascimento"] = dataNasc;
    json["embedding"] = embArray;

    QJsonDocument doc(json);

    QNetworkReply *reply = manager.post(request, doc.toJson());

    // ⭐⭐⭐ AQUI ESTÁ O SEGREDO ⭐⭐⭐
    connect(reply, &QNetworkReply::finished, [reply]()
            {
                qDebug() << "STATUS:" << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                qDebug() << "RESPOSTA:" << reply->readAll();
                reply->deleteLater();
            });
}


void ApiService::sendRegisto(int colaboradorId,
                             const QString &tipo)
{
    QUrl url(baseUrl + "registos");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject json;
    json["colaborador_id"] = colaboradorId;
    json["tipo"] = tipo;

    QNetworkReply *reply = manager.post(
        request,
        QJsonDocument(json).toJson()
        );

    connect(reply, &QNetworkReply::finished, [reply]() {
        qDebug() << "🌐 Registo enviado para API";
        reply->deleteLater();
    });
}
