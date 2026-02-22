#include "ShopRepository.h"

QSqlQuery ShopRepository::getAll()
{
    return db().execQuery(
        "SELECT s.id, s.name, s.address, s.is_open, s.director_margin, "
        "  COALESCE(d.full_name,'—') "
        "FROM shops s "
        "LEFT JOIN employees d ON d.shop_id = s.id AND d.role='director' AND d.is_active "
        "ORDER BY s.id"
    );
}

bool ShopRepository::setOpen(int shopId, bool open)
{
    auto q = prepare("UPDATE shops SET is_open = :open WHERE id = :id");
    q.bindValue(":open", open);
    q.bindValue(":id",   shopId);
    return execAndCheck(q);
}

bool ShopRepository::setMargin(int shopId, double margin)
{
    auto q = prepare("UPDATE shops SET director_margin = :m WHERE id = :id");
    q.bindValue(":m",  margin);
    q.bindValue(":id", shopId);
    return execAndCheck(q);
}
