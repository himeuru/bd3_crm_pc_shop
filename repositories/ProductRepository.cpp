#include "ProductRepository.h"
#include <QSqlQuery>
#include <QSqlRecord>

QList<Product> ProductRepository::search(const QString& text, int shopId)
{
    QList<Product> result;
    QString sql;
    if (text.trimmed().isEmpty()) {
        sql = QString(
            "SELECT p.id, p.name, p.category, p.cost_price "
            "FROM warehouse_stock ws "
            "JOIN warehouses w ON w.id = ws.warehouse_id AND w.shop_id = %1 AND w.type = 'micro' "
            "JOIN products   p ON p.id = ws.product_id AND p.is_active "
            "WHERE ws.quantity > 0 ORDER BY p.category, p.name"
        ).arg(shopId);
    } else {
        sql = QString(
            "SELECT p.id, p.name, p.category, p.cost_price "
            "FROM warehouse_stock ws "
            "JOIN warehouses w ON w.id = ws.warehouse_id AND w.shop_id = %1 AND w.type = 'micro' "
            "JOIN products   p ON p.id = ws.product_id AND p.is_active "
            "WHERE ws.quantity > 0 "
            "  AND (p.search_vec @@ plainto_tsquery('russian', '%2') OR lower(p.name) %% lower('%2')) "
            "ORDER BY p.name"
        ).arg(shopId).arg(text);
    }
    auto q = db().execQuery(sql);
    while (q.next()) {
        Product p;
        p.id = q.value(0).toInt();
        result.append(p);
    }
    return result;
}

QList<Product> ProductRepository::getActiveByShop(int shopId)
{
    return search("", shopId);
}
