#pragma once
#include "BaseRepository.h"
#include <QSqlQuery>

class ShopRepository : public BaseRepository {
public:
    QSqlQuery getAll();
    bool setOpen(int shopId, bool open);
    bool setMargin(int shopId, double margin);
};
