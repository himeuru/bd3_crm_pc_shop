#pragma once
#include "../repositories/EmployeeRepository.h"
#include <QString>

class StaffController {
public:
    int  hire(const QString& fullName, const QString& role, double salary,
              double salaryPct, const QString& login, const QString& password);
    bool fire(int employeeId);
    bool updateSalary(int employeeId, double newSalary);
};
