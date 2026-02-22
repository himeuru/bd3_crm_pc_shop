#pragma once
#include <QString>

struct DbConfig {
    QString host     = "127.0.0.1";
    int     port     = 5432;
    QString dbName   = "crm_pc_shop";
    QString user     = "postgres";
    QString password = "123";
};

class AppConfig {
public:
    static AppConfig& instance();

    void load();
    void save();

    DbConfig db;

private:
    AppConfig() = default;
};
