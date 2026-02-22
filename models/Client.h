#pragma once
#include <QString>

struct Client {
    int     id           = 0;
    QString fullName;
    QString phone;
    QString email;
    QString login;
    double  discountPct  = 0.0;
    QString registeredAt;
};
