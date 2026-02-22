#include "ClientRepository.h"
#include <QSqlQuery>
#include <QSqlError>

ClientRepository::ClientInfo ClientRepository::findByLoginOrPhone(const QString& q)
{
    ClientInfo info;
    auto query = prepare(
        "SELECT id, full_name, discount_pct FROM clients "
        "WHERE login = :q OR phone = :q LIMIT 1");
    query.bindValue(":q", q);
    if (!execAndCheck(query) || !query.next()) return info;
    info.id       = query.value(0).toInt();
    info.fullName = query.value(1).toString();
    info.discount = query.value(2).toDouble();
    info.found    = true;
    return info;
}

bool ClientRepository::registerClient(
    const QString& fullName, const QString& phone,
    const QString& email,    const QString& login,
    const QString& password, double discountPct)
{
    auto q = prepare(
        "INSERT INTO clients (full_name, phone, email, login, password_hash, discount_pct) "
        "VALUES (:name, :phone, :email, :login, crypt(:pass, gen_salt('bf', 12)), :disc)");
    q.bindValue(":name",  fullName);
    q.bindValue(":phone", phone);
    q.bindValue(":email", email);
    q.bindValue(":login", login);
    q.bindValue(":pass",  password);
    q.bindValue(":disc",  discountPct);
    return execAndCheck(q);
}

bool ClientRepository::setDiscount(int clientId, double discountPct)
{
    auto q = prepare("UPDATE clients SET discount_pct = :disc WHERE id = :id");
    q.bindValue(":disc", discountPct);
    q.bindValue(":id",   clientId);
    return execAndCheck(q);
}
