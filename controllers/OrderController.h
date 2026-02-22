#pragma once
#include "../repositories/OrderRepository.h"
#include <QVector>
#include <QString>

class OrderController {
public:
    struct CartItem {
        int     productId;
        QString name;
        QString category;
        double  salePrice;
        int     qty;
    };
    int  createOrder(int clientId, const QVector<CartItem>& items);
    bool cancelOrder(int orderId);
};
