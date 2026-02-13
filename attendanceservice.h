#ifndef ATTENDANCESERVICE_H
#define ATTENDANCESERVICE_H

#include <QString>
#include <vector>
#include "databasemanager.h"

class AttendanceService
{
public:
    AttendanceService(DatabaseManager *db);

    int createColaborador(const QString &nome,
                          const QString &apelido,
                          const QString &dataNasc);

    bool saveEmbedding(int colaboradorId,
                       const std::vector<float> &embedding);

    int createColaboradorWithEmbedding(
        const QString &nome,
        const QString &apelido,
        const QString &dataNasc,
        const std::vector<float> &embedding);

    QString getLastRegisterType(int colaboradorId);
    void insertRegister(int colaboradorId, const QString &tipo);

private:
    DatabaseManager *database;
};
#endif
