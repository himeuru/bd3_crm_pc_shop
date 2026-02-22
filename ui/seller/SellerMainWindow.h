#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>
#include <QLabel>

class NewOrderTab;
class ClientsTab;
class StockTab;
class SalaryTab;

class SellerMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit SellerMainWindow(QWidget* parent = nullptr);
private slots:
    void refreshCurrentTab();
    void onLogout();
private:
    void buildUi();
    QTabWidget*  m_tabs;
    NewOrderTab* m_newOrderTab;
    ClientsTab*  m_clientsTab;
    StockTab*    m_stockTab;
    SalaryTab*   m_salaryTab;
    QTimer*      m_refreshTimer;
};
