#include "StaffController.h"
#include "../core/Session.h"

int StaffController::hire(const QString& fullName, const QString& role,
                           double salary, double salaryPct,
                           const QString& login, const QString& password)
{
    EmployeeRepository repo;
    return repo.hire(Session::instance().shopId, fullName, role,
                     salary, salaryPct, login, password);
}

bool StaffController::fire(int employeeId)
{
    EmployeeRepository repo;
    return repo.fire(employeeId, Session::instance().userId);
}

bool StaffController::updateSalary(int employeeId, double newSalary)
{
    EmployeeRepository repo;
    return repo.updateSalary(employeeId, newSalary, Session::instance().userId);
}
