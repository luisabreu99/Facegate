#include "database.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

static void logErr(const char* where, const QSqlQuery& q) {
    if (q.lastError().isValid())
        qDebug() << where << " => " << q.lastError().text();
}

bool Database::inicializar()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("colaboradores.db"); // fica ao lado do .exe

    if (!db.open()) {
        qDebug() << "Erro ao abrir a BD:" << db.lastError().text();
        return false;
    }

    QSqlQuery q;

    // colaboradores
    q.exec("CREATE TABLE IF NOT EXISTS colaboradores ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "nome TEXT,"
           "apelido TEXT,"
           "data_nascimento TEXT,"
           "pasta_fotos TEXT)");
    logErr("CREATE colaboradores", q);

    // rostos
    q.exec("CREATE TABLE IF NOT EXISTS rostos ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "colaborador_id INTEGER,"
           "caminho_foto TEXT,"
           "encoding BLOB,"
           "FOREIGN KEY(colaborador_id) REFERENCES colaboradores(id))");
    logErr("CREATE rostos", q);

    // entradas/saidas
    q.exec("CREATE TABLE IF NOT EXISTS entradas_saidas ("
           "id INTEGER PRIMARY KEY AUTOINCREMENT,"
           "colaborador_id INTEGER,"
           "tipo TEXT,"
           "data_hora TEXT DEFAULT CURRENT_TIMESTAMP,"
           "FOREIGN KEY(colaborador_id) REFERENCES colaboradores(id))");
    logErr("CREATE entradas_saidas", q);

    return true;
}

int Database::adicionarColaborador(const QString &nome,
                                   const QString &apelido,
                                   const QString &dataNascimento,
                                   const QString &pasta)
{
    QSqlQuery q;
    q.prepare("INSERT INTO colaboradores (nome, apelido, data_nascimento, pasta_fotos) "
              "VALUES (?, ?, ?, ?)");
    q.addBindValue(nome);
    q.addBindValue(apelido);
    q.addBindValue(dataNascimento);
    q.addBindValue(pasta);

    if (!q.exec()) {
        logErr("INSERT colaborador", q);
        return -1;
    }
    return q.lastInsertId().toInt();
}

void Database::adicionarFoto(int colaboradorId, const QString &caminhoFoto)
{
    adicionarFoto(colaboradorId, caminhoFoto, QByteArray{});
}

void Database::adicionarFoto(int colaboradorId,
                             const QString &caminhoFoto,
                             const QByteArray &encoding)
{
    QSqlQuery q;
    q.prepare("INSERT INTO rostos (colaborador_id, caminho_foto, encoding) "
              "VALUES (?, ?, ?)");
    q.addBindValue(colaboradorId);
    q.addBindValue(caminhoFoto);
    q.addBindValue(encoding);

    if (!q.exec()) {
        logErr("INSERT rosto", q);
    }
}

// 👉 NOVA FUNÇÃO: buscar encodes da BD
QVector<QPair<int, QByteArray>> Database::obterEncodings()
{
    QVector<QPair<int, QByteArray>> encodings;

    QSqlQuery q("SELECT colaborador_id, encoding FROM rostos");
    if (!q.exec()) {
        logErr("SELECT encodings", q);
        return encodings;
    }

    while (q.next()) {
        int id = q.value(0).toInt();
        QByteArray blob = q.value(1).toByteArray();
        encodings.append(qMakePair(id, blob));
    }

    return encodings;
}
QString Database::obterUltimaMarcacao(int colaboradorId)
{
    QSqlQuery q;
    q.prepare("SELECT tipo FROM entradas_saidas "
              "WHERE colaborador_id = ? "
              "ORDER BY datetime(data_hora) DESC LIMIT 1");
    q.addBindValue(colaboradorId);

    if (!q.exec()) {
        logErr("SELECT ultima marcacao", q);
        return QString();
    }

    if (q.next()) {
        return q.value(0).toString();
    }

    return QString(); // não tem registos
}

bool Database::registarMarcacao(int colaboradorId, const QString &tipo)
{
    QSqlQuery q;
    q.prepare("INSERT INTO entradas_saidas (colaborador_id, tipo, data_hora) "
              "VALUES (?, ?, datetime('now','localtime'))");
    q.addBindValue(colaboradorId);
    q.addBindValue(tipo);

    if (!q.exec()) {
        logErr("INSERT marcacao", q);
        return false;
    }

    return true;
}
