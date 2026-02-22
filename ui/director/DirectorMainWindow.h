#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QTimer>

class ShopsTab;
class StaffTab;
class OrdersTab;
class WarehouseTab;
class ReportsTab;

class DirectorMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit DirectorMainWindow(QWidget* parent = nullptr);

private slots:
    void refreshCurrentTab();
    void onLogout();

private:
    void buildUi();
    void buildStatusBar();

    QTabWidget*   m_tabs;
    ShopsTab*     m_shopsTab;
    StaffTab*     m_staffTab;
    OrdersTab*    m_ordersTab;
    WarehouseTab* m_warehouseTab;
    ReportsTab*   m_reportsTab;
    QTimer*       m_refreshTimer;
    QLabel*       m_statusLabel;
};
