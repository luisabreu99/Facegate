#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QString>
#include <sqlite3.h>

class DatabaseManager
{
public:
    DatabaseManager();
    ~DatabaseManager();

    bool open(const QString &path);
    sqlite3* getDB();

private:
    sqlite3 *db = nullptr;
};

#endif
