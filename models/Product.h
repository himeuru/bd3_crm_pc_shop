#pragma once
#include <QString>

struct Product {
    int     id          = 0;
    QString name;
    QString category;
    QString description;
    double  costPrice   = 0.0;
    bool    isActive    = true;
};
