#include "OrderController.h"
#include "../core/Session.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

int OrderController::createOrder(int clientId, const QVector<CartItem>& items)
{
    QJsonArray arr;
    for (const auto& item : items) {
        QJsonObject obj;
        obj["product_id"] = item.productId;
        obj["qty"]        = item.qty;
        arr.append(obj);
    }
    QByteArray json = QJsonDocument(arr).toJson(QJsonDocument::Compact);
    auto& s = Session::instance();
    OrderRepository repo;
    return repo.createOrder(s.shopId, clientId, s.userId, json);
}

bool OrderController::cancelOrder(int orderId)
{
    OrderRepository repo;
    return repo.cancelOrder(orderId, Session::instance().userId);
}
