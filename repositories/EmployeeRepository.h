#pragma once
#include "BaseRepository.h"
#include <QSqlQuery>

class EmployeeRepository : public BaseRepository {
public:
    QSqlQuery getByShop(int shopId);
    int  hire(int shopId, const QString& fullName, const QString& role,
              double salary, double salaryPct, const QString& login, const QString& password);
    bool fire(int employeeId, int directorId);
    bool updateSalary(int employeeId, double newSalary, int directorId);
};
