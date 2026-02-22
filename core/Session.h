#pragma once
#include <QString>

enum class UserRole {
    None,
    Director,
    Seller,
    Client
};

class Session {
public:
    static Session& instance();

    int      userId   = -1;
    int      shopId   = -1;
    QString  fullName;
    UserRole role     = UserRole::None;

    bool isLoggedIn()  const { return userId > 0; }
    bool isDirector()  const { return role == UserRole::Director; }
    bool isSeller()    const { return role == UserRole::Seller; }
    bool isClient()    const { return role == UserRole::Client; }

    void clear() {
        userId = -1;
        shopId = -1;
        fullName.clear();
        role = UserRole::None;
    }

private:
    Session() = default;
};
