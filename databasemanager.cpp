#include "databasemanager.h"
#include <QDebug>

DatabaseManager::DatabaseManager() {}

DatabaseManager::~DatabaseManager()
{
    if (db)
        sqlite3_close(db);
}

bool DatabaseManager::open(const QString &path)
{
    if (sqlite3_open(path.toStdString().c_str(), &db) != SQLITE_OK)
    {
        qDebug() << "Erro ao abrir base de dados";
        return false;
    }
      sqlite3_exec(db, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);

    qDebug() << "Base de dados aberta com sucesso";
    return true;
}

sqlite3* DatabaseManager::getDB()
{
    return db;
}
