#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"
#include <QPushButton>
#include <QSpinBox>

class WarehouseTab : public QWidget {
    Q_OBJECT
public:
    explicit WarehouseTab(QWidget* parent = nullptr);
    void reload();
private slots:
    void onReplenish();
private:
    void buildUi();
    SqlTableWidget* m_mainTable;
    SqlTableWidget* m_microTable;
    QPushButton*    m_btnReplenish;
    QSpinBox*       m_spinFillTo;
};
