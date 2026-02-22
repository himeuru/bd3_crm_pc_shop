#pragma once
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>

class DatabaseManager {
public:
    static DatabaseManager& instance();

    bool connect(const QString& host, int port,
                 const QString& dbName,
                 const QString& user,
                 const QString& password);

    bool isConnected() const;
    void disconnect();

    QSqlDatabase& db();

    bool setSessionUser(int userId);

    bool        execVoid(const QString& sql);
    QSqlQuery   execQuery(const QString& sql);

    bool transaction();
    bool commit();
    bool rollback();

    QString lastError() const;

private:
    DatabaseManager() = default;
    QSqlDatabase m_db;
    QString      m_lastError;
};
