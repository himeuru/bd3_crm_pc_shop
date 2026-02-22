#pragma once
#include <QString>
#include <QList>

struct OrderItem {
    int     productId = 0;
    QString productName;
    QString category;
    int     quantity  = 0;
    double  costPrice = 0.0;
    double  salePrice = 0.0;
};

struct Order {
    int     id           = 0;
    int     shopId       = 0;
    int     sellerId     = 0;
    int     clientId     = 0;
    QString clientName;
    QString sellerName;
    QString status;
    double  discountPct  = 0.0;
    double  totalAmount  = 0.0;
    QString createdAt;
    QString cancelledAt;
    QList<OrderItem> items;
};
