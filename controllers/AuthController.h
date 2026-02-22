#pragma once
#include "../core/Session.h"
#include "../repositories/AuthRepository.h"
#include <QString>
#include <QWidget>

class AuthController {
public:
    struct LoginResult {
        bool    ok       = false;
        QString role;
        QString errorMsg;
    };
    LoginResult login(const QString& login, const QString& password);
    bool changePassword(const QString& oldPass, const QString& newPass);
    static void openWindowForRole(const QString& role);
};
