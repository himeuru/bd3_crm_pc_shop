#pragma once
#include "BaseRepository.h"
#include "../models/Client.h"
#include <QString>

class ClientRepository : public BaseRepository {
public:
    struct ClientInfo {
        int     id       = -1;
        QString fullName;
        double  discount = 0.0;
        bool    found    = false;
    };
    ClientInfo findByLoginOrPhone(const QString& query);
    bool registerClient(const QString& fullName, const QString& phone,
                        const QString& email,    const QString& login,
                        const QString& password, double discountPct);
    bool setDiscount(int clientId, double discountPct);
};
