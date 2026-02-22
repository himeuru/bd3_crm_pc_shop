#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"

class SalaryTab : public QWidget {
    Q_OBJECT
public:
    explicit SalaryTab(QWidget* parent = nullptr);
    void reload();
private:
    void buildUi();
    SqlTableWidget* m_monthlyTable;
    SqlTableWidget* m_ordersTable;
};
