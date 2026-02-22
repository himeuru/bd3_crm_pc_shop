#pragma once
#include <QString>

struct StockItem {
    int     warehouseId   = 0;
    QString warehouseType;
    int     shopId        = 0;
    int     productId     = 0;
    QString productName;
    QString category;
    int     quantity      = 0;
    double  costPrice     = 0.0;
    QString updatedAt;
};
