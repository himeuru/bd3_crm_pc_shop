#include "EmployeeRepository.h"

QSqlQuery EmployeeRepository::getByShop(int shopId)
{
    return db().execQuery(QString(
        "SELECT id, full_name, role, salary, salary_pct, is_active, "
        "  to_char(hired_at,'DD.MM.YYYY') "
        "FROM employees WHERE shop_id = %1 ORDER BY role DESC, is_active DESC, full_name"
    ).arg(shopId));
}

int EmployeeRepository::hire(int shopId, const QString& fullName, const QString& role,
                              double salary, double salaryPct,
                              const QString& login, const QString& password)
{
    auto q = prepare("SELECT hire_employee(:shop,:name,:role,:salary,:pct,:login,:pass)");
    q.bindValue(":shop",   shopId);
    q.bindValue(":name",   fullName);
    q.bindValue(":role",   role);
    q.bindValue(":salary", salary);
    q.bindValue(":pct",    salaryPct);
    q.bindValue(":login",  login);
    q.bindValue(":pass",   password);
    if (!execAndCheck(q) || !q.next()) return -1;
    return q.value(0).toInt();
}

bool EmployeeRepository::fire(int employeeId, int directorId)
{
    auto q = prepare("SELECT fire_employee(:emp, :dir)");
    q.bindValue(":emp", employeeId);
    q.bindValue(":dir", directorId);
    return execAndCheck(q);
}

bool EmployeeRepository::updateSalary(int employeeId, double newSalary, int directorId)
{
    auto q = prepare("SELECT update_salary(:emp, :sal, :dir)");
    q.bindValue(":emp", employeeId);
    q.bindValue(":sal", newSalary);
    q.bindValue(":dir", directorId);
    return execAndCheck(q);
}
