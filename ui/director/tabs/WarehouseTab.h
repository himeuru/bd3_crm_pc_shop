#pragma once
#include <QWidget>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include "../../widgets/SqlTableWidget.h"

class WarehouseTab : public QWidget {
    Q_OBJECT
public:
    explicit WarehouseTab(QWidget* parent = nullptr);

public slots:
    void reload();

private slots:
    void onReplenish();

private:
    void buildUi();

    SqlTableWidget* m_mainTable    = nullptr;
    SqlTableWidget* m_microTable   = nullptr;
    QSpinBox*       m_spinFillTo   = nullptr;
    QPushButton*    m_btnReplenish = nullptr;
    QLabel*         m_statusLabel  = nullptr;
};