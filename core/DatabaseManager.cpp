#include "DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager inst;
    return inst;
}

bool DatabaseManager::connect(const QString& host, int port,
                               const QString& dbName,
                               const QString& user,
                               const QString& password)
{
    if (m_db.isOpen())
        m_db.close();

    if (QSqlDatabase::contains("crm_connection"))
        QSqlDatabase::removeDatabase("crm_connection");

    m_db = QSqlDatabase::addDatabase("QPSQL", "crm_connection");
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);
    m_db.setConnectOptions("connect_timeout=5;application_name=crm_pc_shop");

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qWarning() << "[DB] Connection failed:" << m_lastError;
        return false;
    }

    qDebug() << "[DB] Connected to" << dbName << "@" << host << ":" << port;
    return true;
}

bool DatabaseManager::isConnected() const
{
    return m_db.isOpen();
}

void DatabaseManager::disconnect()
{
    if (m_db.isOpen()) {
        m_db.close();
        qDebug() << "[DB] Disconnected";
    }
}

QSqlDatabase& DatabaseManager::db()
{
    return m_db;
}

bool DatabaseManager::setSessionUser(int userId)
{
    QSqlQuery q(m_db);
    q.prepare("SET LOCAL app.current_user_id = :id");
    q.bindValue(":id", userId);
    if (!q.exec()) {
        m_lastError = q.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseManager::execVoid(const QString& sql)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        qWarning() << "[DB] execVoid failed:" << m_lastError;
        return false;
    }
    return true;
}

QSqlQuery DatabaseManager::execQuery(const QString& sql)
{
    QSqlQuery q(m_db);
    if (!q.exec(sql)) {
        m_lastError = q.lastError().text();
        qWarning() << "[DB] execQuery failed:" << m_lastError;
    }
    return q;
}

bool DatabaseManager::transaction()  { return m_db.transaction(); }
bool DatabaseManager::commit()       { return m_db.commit(); }
bool DatabaseManager::rollback()     { return m_db.rollback(); }

QString DatabaseManager::lastError() const { return m_lastError; }
