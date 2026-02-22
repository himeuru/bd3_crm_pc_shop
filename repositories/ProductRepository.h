#pragma once
#include "BaseRepository.h"
#include "../models/Product.h"
#include <QList>
#include <QString>

class ProductRepository : public BaseRepository {
public:
    QList<Product> search(const QString& text, int shopId);
    QList<Product> getActiveByShop(int shopId);
};
