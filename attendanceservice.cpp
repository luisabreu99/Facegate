#include "attendanceservice.h"
#include <sqlite3.h>
#include <QDebug>

AttendanceService::AttendanceService(DatabaseManager *db)
{
    database = db;
}







int AttendanceService::createColaborador(const QString &nome,
                                         const QString &apelido,
                                         const QString &dataNasc)
{
    sqlite3 *db = database->getDB();

    const char *sql =
        "INSERT INTO colaboradores (nome, apelido, data_nascimento) "
        "VALUES (?, ?, ?)";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, nome.toStdString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, apelido.toStdString().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, dataNasc.toStdString().c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        qDebug() << "❌ ERRO SQLITE criar colaborador:"
                 << sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return -1;
    }

    sqlite3_finalize(stmt);
    return (int)sqlite3_last_insert_rowid(db);
}








bool AttendanceService::saveEmbedding(int colaboradorId,
                                      const std::vector<float> &embedding)
{
    sqlite3 *db = database->getDB();

    const char *sql =
        "INSERT INTO rostos (colaborador_id, encoding) VALUES (?, ?)";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int(stmt, 1, colaboradorId);
    sqlite3_bind_blob(stmt, 2, embedding.data(),
                      128*sizeof(float), SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);

    if (rc != SQLITE_DONE)
    {
        qDebug() << "❌ ERRO SQLITE guardar embedding:"
                 << sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);

    return rc;
}








void AttendanceService::insertRegister(int colaboradorId, const QString &tipo)
{
    sqlite3 *db = database->getDB();

    const char *sql =
        "INSERT INTO entradas_saidas (colaborador_id, tipo, data_hora) "
        "VALUES (?, ?, datetime('now'))";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);

    sqlite3_bind_int(stmt, 1, colaboradorId);
    sqlite3_bind_text(stmt, 2, tipo.toStdString().c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
int AttendanceService::createColaboradorWithEmbedding(
    const QString &nome,
    const QString &apelido,
    const QString &dataNasc,
    const std::vector<float> &embedding)
{
    sqlite3 *db = database->getDB();

    sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);

    int id = createColaborador(nome, apelido, dataNasc);

    if (id == -1)
    {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return -1;
    }

    if (!saveEmbedding(id, embedding))
    {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return -1;
    }

    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);

    qDebug() << "✔ Colaborador criado com ID:" << id;
    return id;
}




QString AttendanceService::getLastRegisterType(int colaboradorId)
{
    sqlite3 *db = database->getDB();

    const char *sql =
        "SELECT tipo FROM entradas_saidas "
        "WHERE colaborador_id = ? "
        "ORDER BY data_hora DESC LIMIT 1";

    sqlite3_stmt *stmt = nullptr;
    sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, colaboradorId);

    QString tipo = "";

    if (sqlite3_step(stmt) == SQLITE_ROW)
        tipo = QString(reinterpret_cast<const char*>(sqlite3_column_text(stmt,0)));

    sqlite3_finalize(stmt);
    return tipo;
}
