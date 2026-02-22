#pragma once
#include "BaseRepository.h"
#include <QSqlQuery>

class WarehouseRepository : public BaseRepository {
public:
    QSqlQuery getMainStock();
    QSqlQuery getMicroStock(int shopId);
    QSqlQuery getTransferLog(int shopId, const QString& from, const QString& to);
    bool replenish(int shopId, int userId, int fillTo);
};
