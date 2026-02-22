#pragma once
#include "../core/DatabaseManager.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>

class BaseRepository {
protected:
    DatabaseManager& db() { return DatabaseManager::instance(); }

    QSqlQuery prepare(const QString& sql) {
        QSqlQuery q(DatabaseManager::instance().db());
        if (!q.prepare(sql)) {
            qWarning() << "[Repo] prepare failed:" << q.lastError().text();
        }
        return q;
    }

    bool execAndCheck(QSqlQuery& q) {
        if (!q.exec()) {
            qWarning() << "[Repo] exec failed:" << q.lastError().text();
            return false;
        }
        return true;
    }
};
