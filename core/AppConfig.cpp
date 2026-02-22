#include "AppConfig.h"
#include <QSettings>

AppConfig& AppConfig::instance()
{
    static AppConfig inst;
    return inst;
}

void AppConfig::load()
{
    QSettings s("PCShopCorp", "CRM_PC_Shop");
    db.host     = s.value("db/host",     db.host).toString();
    db.port     = s.value("db/port",     db.port).toInt();
    db.dbName   = s.value("db/name",     db.dbName).toString();
    db.user     = s.value("db/user",     db.user).toString();
    db.password = s.value("db/password", db.password).toString();
}

void AppConfig::save()
{
    QSettings s("PCShopCorp", "CRM_PC_Shop");
    s.setValue("db/host",     db.host);
    s.setValue("db/port",     db.port);
    s.setValue("db/name",     db.dbName);
    s.setValue("db/user",     db.user);
    s.setValue("db/password", db.password);
}
