#include "SellerMainWindow.h"
#include "tabs/NewOrderTab.h"
#include "tabs/ClientsTab.h"
#include "tabs/StockTab.h"
#include "tabs/SalaryTab.h"
#include "../../core/Session.h"
#include "../../ui/LoginWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QMessageBox>
#include <QWidget>

SellerMainWindow::SellerMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("CRM PC Shop — Продавец");
    setMinimumSize(900, 650);
    resize(1100, 700);
    buildUi();

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &SellerMainWindow::refreshCurrentTab);
    m_refreshTimer->start(15000);
}

void SellerMainWindow::buildUi()
{
    m_newOrderTab = new NewOrderTab(this);
    m_clientsTab  = new ClientsTab(this);
    m_stockTab    = new StockTab(this);
    m_salaryTab   = new SalaryTab(this);

    m_tabs = new QTabWidget(this);
    m_tabs->addTab(m_newOrderTab, "🛒  Новый заказ");
    m_tabs->addTab(m_clientsTab,  "👤  Клиенты");
    m_tabs->addTab(m_stockTab,    "📦  Склад");
    m_tabs->addTab(m_salaryTab,   "💰  Зарплата");

    auto* header  = new QWidget(this);
    header->setStyleSheet("background: #0a0a12; border-bottom: 1px solid #3b3d57;");
    auto* hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(16, 8, 16, 8);

    auto* nameLabel = new QLabel(
        QString("⚙  CRM PC Shop  |  Продавец: <b>%1</b>")
            .arg(Session::instance().fullName), header);
    nameLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");

    auto* btnLogout = new QPushButton("Выйти", header);
    btnLogout->setObjectName("btnDanger");
    btnLogout->setStyleSheet("background:#f7768e; color:#ffffff; font-weight:bold; border-radius:6px;");
    btnLogout->setFixedSize(90, 30);

    hLayout->addWidget(nameLabel);
    hLayout->addStretch();
    hLayout->addWidget(btnLogout);

    auto* wrapper = new QWidget(this);
    auto* wLayout = new QVBoxLayout(wrapper);
    wLayout->setContentsMargins(0, 0, 0, 0);
    wLayout->setSpacing(0);
    wLayout->addWidget(header);
    wLayout->addWidget(m_tabs);

    setCentralWidget(wrapper);

    connect(btnLogout, &QPushButton::clicked, this, &SellerMainWindow::onLogout);

    // Обновлять склад и зарплату при переключении на соответствующую вкладку
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int idx) {
        switch (idx) {
        case 0: m_newOrderTab->reloadProducts(); break;
        case 2: m_stockTab->reload();   break;
        case 3: m_salaryTab->reload();  break;
        }
    });
}

void SellerMainWindow::refreshCurrentTab()
{
    switch (m_tabs->currentIndex()) {
    case 2: m_stockTab->reload();  break;
    case 3: m_salaryTab->reload(); break;
    }
}

void SellerMainWindow::onLogout()
{
    if (QMessageBox::question(this, "Выход", "Выйти из системы?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    m_refreshTimer->stop();
    Session::instance().clear();
    (new LoginWindow())->show();
    close();
}