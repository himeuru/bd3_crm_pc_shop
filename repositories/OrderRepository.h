#pragma once
#include "BaseRepository.h"
#include <QSqlQuery>

class OrderRepository : public BaseRepository {
public:
    QSqlQuery getByShop(int shopId, const QString& from, const QString& to, const QString& status);
    QSqlQuery getByClient(int clientId);
    QSqlQuery getItems(int orderId);
    int  createOrder(int shopId, int clientId, int sellerId, const QByteArray& itemsJson);
    bool cancelOrder(int orderId, int directorId);
};
