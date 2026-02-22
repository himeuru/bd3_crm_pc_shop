#pragma once
#include <QString>

struct Employee {
    int     id         = 0;
    int     shopId     = 0;
    QString fullName;
    QString role;
    double  salary     = 0.0;
    double  salaryPct  = 0.10;
    QString login;
    bool    isActive   = true;
    QString hiredAt;
};
