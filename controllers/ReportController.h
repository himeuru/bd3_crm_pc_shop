#pragma once
#include <QString>

class ReportController {
public:
    enum class ReportType {
        MonthlySales,
        Salaries,
        TopProducts,
        TopCategories,
        SalaryLog,
        StockLog
    };
    QString buildQuery(ReportType type, int shopId,
                       const QString& from, const QString& to);
};
