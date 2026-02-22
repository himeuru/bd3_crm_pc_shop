#include "OrderRepository.h"
#include <QSqlError>
#include <QDebug>

QSqlQuery OrderRepository::getByShop(int shopId, const QString& from, const QString& to, const QString& status)
{
    QString clause = (status == "all") ? "" : QString("AND o.status = '%1'").arg(status);
    return db().execQuery(QString(
        "SELECT o.id, to_char(o.created_at,'DD.MM.YYYY HH24:MI'), "
        "  c.full_name, e.full_name, o.total_amount, o.discount_pct, o.status "
        "FROM orders o "
        "JOIN clients c ON c.id = o.client_id "
        "JOIN employees e ON e.id = o.seller_id "
        "WHERE o.shop_id = %1 AND o.created_at::date BETWEEN '%2' AND '%3' %4 "
        "ORDER BY o.created_at DESC"
    ).arg(shopId).arg(from).arg(to).arg(clause));
}

QSqlQuery OrderRepository::getByClient(int clientId)
{
    return db().execQuery(QString(
        "SELECT o.id, to_char(o.created_at,'DD.MM.YYYY HH24:MI'), "
        "  o.total_amount, o.discount_pct, o.status "
        "FROM orders o WHERE o.client_id = %1 ORDER BY o.created_at DESC"
    ).arg(clientId));
}

QSqlQuery OrderRepository::getItems(int orderId)
{
    return db().execQuery(QString(
        "SELECT p.name, oi.product_category, oi.quantity, oi.cost_price, "
        "  oi.sale_price, oi.sale_price * oi.quantity "
        "FROM order_items oi JOIN products p ON p.id = oi.product_id "
        "WHERE oi.order_id = %1 ORDER BY p.name"
    ).arg(orderId));
}

int OrderRepository::createOrder(int shopId, int clientId, int sellerId, const QByteArray& itemsJson)
{
    auto q = prepare("SELECT create_order(:shop, :client, :seller, :items::jsonb)");
    q.bindValue(":shop",   shopId);
    q.bindValue(":client", clientId);
    q.bindValue(":seller", sellerId);
    q.bindValue(":items",  QString::fromUtf8(itemsJson));
    if (!execAndCheck(q) || !q.next()) return -1;
    return q.value(0).toInt();
}

bool OrderRepository::cancelOrder(int orderId, int directorId)
{
    auto q = prepare("SELECT cancel_order(:order, :dir)");
    q.bindValue(":order", orderId);
    q.bindValue(":dir",   directorId);
    return execAndCheck(q);
}
