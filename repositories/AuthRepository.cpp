#include "AuthRepository.h"

AuthRepository::AuthResult AuthRepository::login(
    const QString& login, const QString& password)
{
    AuthResult res;
    auto q = prepare("SELECT user_id, user_role, shop_id, full_name "
                     "FROM authenticate_user(:login, :pass)");
    q.bindValue(":login", login);
    q.bindValue(":pass",  password);

    if (!execAndCheck(q) || !q.next())
        return res;

    res.userId   = q.value("user_id").toInt();
    res.role     = q.value("user_role").toString();
    res.shopId   = q.value("shop_id").toInt();
    res.fullName = q.value("full_name").toString();
    res.ok       = true;
    return res;
}

bool AuthRepository::changePassword(int userId, const QString& role,
                                     const QString& oldPass, const QString& newPass)
{
    auto q = prepare("SELECT change_password(:uid, :role, :old, :new)");
    q.bindValue(":uid",  userId);
    q.bindValue(":role", role);
    q.bindValue(":old",  oldPass);
    q.bindValue(":new",  newPass);

    if (!execAndCheck(q) || !q.next())
        return false;

    return q.value(0).toBool();
}
