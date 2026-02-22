#include "AuthController.h"
#include "../core/DatabaseManager.h"
#include "../ui/director/DirectorMainWindow.h"
#include "../ui/seller/SellerMainWindow.h"
#include "../ui/client/ClientMainWindow.h"

AuthController::LoginResult AuthController::login(const QString& loginStr, const QString& password)
{
    LoginResult result;
    AuthRepository repo;
    auto res = repo.login(loginStr, password);
    if (!res.ok) {
        result.errorMsg = "Неверный логин или пароль";
        return result;
    }
    auto& s  = Session::instance();
    s.userId   = res.userId;
    s.shopId   = res.shopId;
    s.fullName = res.fullName;
    DatabaseManager::instance().setSessionUser(res.userId);

    if      (res.role == "director") s.role = UserRole::Director;
    else if (res.role == "seller")   s.role = UserRole::Seller;
    else                             s.role = UserRole::Client;

    result.ok   = true;
    result.role = res.role;
    return result;
}

bool AuthController::changePassword(const QString& oldPass, const QString& newPass)
{
    AuthRepository repo;
    auto& s   = Session::instance();
    QString role = s.isClient() ? "client" : "employee";
    return repo.changePassword(s.userId, role, oldPass, newPass);
}

void AuthController::openWindowForRole(const QString& role)
{
    QWidget* win = nullptr;
    if      (role == "director") win = new DirectorMainWindow();
    else if (role == "seller")   win = new SellerMainWindow();
    else                         win = new ClientMainWindow();
    win->show();
}
