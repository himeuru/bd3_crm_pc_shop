#include "WarehouseRepository.h"

QSqlQuery WarehouseRepository::getMainStock()
{
    return db().execQuery(
        "SELECT p.name, p.category, ws.quantity, p.cost_price "
        "FROM warehouse_stock ws "
        "JOIN warehouses w ON w.id = ws.warehouse_id AND w.type = 'main' "
        "JOIN products   p ON p.id = ws.product_id "
        "ORDER BY p.category, p.name"
    );
}

QSqlQuery WarehouseRepository::getMicroStock(int shopId)
{
    return db().execQuery(QString(
        "SELECT p.name, p.category, ws.quantity, p.cost_price, "
        "  to_char(ws.updated_at,'DD.MM.YYYY HH24:MI') "
        "FROM warehouse_stock ws "
        "JOIN warehouses w ON w.id = ws.warehouse_id"
        "  AND w.shop_id = %1 AND w.type = 'micro' "
        "JOIN products   p ON p.id = ws.product_id "
        "ORDER BY ws.quantity ASC, p.name"
    ).arg(shopId));
}

QSqlQuery WarehouseRepository::getTransferLog(
    int shopId, const QString& from, const QString& to)
{
    return db().execQuery(QString(
        "SELECT to_char(stl.transferred_at,'DD.MM.YYYY HH24:MI'), "
        "  p.name, stl.quantity, fw.name, tw.name, e.full_name "
        "FROM stock_transfer_log stl "
        "JOIN products   p  ON p.id  = stl.product_id "
        "JOIN warehouses fw ON fw.id = stl.from_warehouse "
        "JOIN warehouses tw ON tw.id = stl.to_warehouse "
        "JOIN employees  e  ON e.id  = stl.initiated_by "
        "WHERE tw.shop_id = %1 "
        "  AND stl.transferred_at::date BETWEEN '%2' AND '%3' "
        "ORDER BY stl.transferred_at DESC"
    ).arg(shopId).arg(from).arg(to));
}

bool WarehouseRepository::replenish(int shopId, int userId, int fillTo)
{
    // ИСПРАВЛЕНИЕ: используем execQuery() напрямую без prepare()/bindValue().
    // Qt QPSQL конфликтует при нескольких prepare() в одном соединении.
    auto q = db().execQuery(QString(
        "SELECT replenish_micro_warehouse(%1, %2, %3)"
    ).arg(shopId).arg(userId).arg(fillTo));
    return q.isActive();
}
