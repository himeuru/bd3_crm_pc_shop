#pragma once
#include "BaseRepository.h"
#include <QString>

class AuthRepository : public BaseRepository {
public:
    struct AuthResult {
        int     userId  = -1;
        QString role;
        int     shopId  = -1;
        QString fullName;
        bool    ok      = false;
    };

    AuthResult login(const QString& login, const QString& password);
    bool       changePassword(int userId, const QString& role,
                              const QString& oldPass, const QString& newPass);
};
