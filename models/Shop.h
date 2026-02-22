#pragma once
#include <QString>

struct Shop {
    int     id             = 0;
    QString name;
    QString address;
    bool    isOpen         = true;
    double  directorMargin = 0.10;
    QString directorName;
    int     sellersCount   = 0;
    double  totalRevenue   = 0.0;
};
