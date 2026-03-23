#include "DirectorMainWindow.h"
#include "tabs/ShopsTab.h"
#include "tabs/StaffTab.h"
#include "tabs/OrdersTab.h"
#include "tabs/WarehouseTab.h"
#include "tabs/ReportsTab.h"
#include "../../core/Session.h"
#include "../../core/DatabaseManager.h"
#include "../../ui/LoginWindow.h"
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QLabel>
#include <QHBoxLayout>
#include <QWidget>
#include <QPushButton>
#include <QMessageBox>

DirectorMainWindow::DirectorMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("CRM PC Shop — Директор");
    setMinimumSize(1100, 700);
    resize(1280, 800);
    buildUi();
    buildStatusBar();

    m_refreshTimer = new QTimer(this);
    connect(m_refreshTimer, &QTimer::timeout, this, &DirectorMainWindow::refreshCurrentTab);
    m_refreshTimer->start(15000);
}

void DirectorMainWindow::buildUi()
{
    m_tabs = new QTabWidget(this);

    m_shopsTab     = new ShopsTab(this);
    m_staffTab     = new StaffTab(this);
    m_ordersTab    = new OrdersTab(this);
    m_warehouseTab = new WarehouseTab(this);
    m_reportsTab   = new ReportsTab(this);

    m_tabs->addTab(m_shopsTab,     "🏪  Магазины");
    m_tabs->addTab(m_staffTab,     "👥  Сотрудники");
    m_tabs->addTab(m_ordersTab,    "📦  Заказы");
    m_tabs->addTab(m_warehouseTab, "🗄  Склад");
    m_tabs->addTab(m_reportsTab,   "📊  Отчёты");

    setCentralWidget(m_tabs);

    auto* header   = new QWidget(this);
    auto* hLayout  = new QHBoxLayout(header);
    hLayout->setContentsMargins(16, 8, 16, 8);
    header->setStyleSheet("background: #0a0a12; border-bottom: 1px solid #3b3d57;");

    auto* nameLabel = new QLabel(
        QString("⚙  CRM PC Shop  |  Директор: <b>%1</b>")
            .arg(Session::instance().fullName),
        header
        );
    nameLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");

    auto* btnLogout = new QPushButton("Выйти", header);
    btnLogout->setObjectName("btnDanger");
    btnLogout->setStyleSheet("background:#f7768e; color:#ffffff; font-weight:bold; border-radius:6px;");
    btnLogout->setFixedWidth(90);
    btnLogout->setFixedHeight(30);

    hLayout->addWidget(nameLabel);
    hLayout->addStretch();
    hLayout->addWidget(btnLogout);

    connect(btnLogout, &QPushButton::clicked, this, &DirectorMainWindow::onLogout);

    // Обновлять данные при переключении на вкладку
    connect(m_tabs, &QTabWidget::currentChanged, this, [this](int idx) {
        switch (idx) {
        case 0: m_shopsTab->reload();     break;
        case 1: m_staffTab->reload();     break;
        case 2: m_ordersTab->reload();    break;
        case 3: m_warehouseTab->reload(); break;
        }
    });

    auto* wrapper = new QWidget(this);
    auto* wLayout = new QVBoxLayout(wrapper);
    wLayout->setContentsMargins(0, 0, 0, 0);
    wLayout->setSpacing(0);
    wLayout->addWidget(header);
    wLayout->addWidget(m_tabs);

    setCentralWidget(wrapper);
}

void DirectorMainWindow::buildStatusBar()
{
    m_statusLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_statusLabel);
    statusBar()->showMessage("Готово");
}

void DirectorMainWindow::refreshCurrentTab()
{
    switch (m_tabs->currentIndex()) {
    case 0: m_shopsTab->reload();     break;
    case 1: m_staffTab->reload();     break;
    case 2: m_ordersTab->reload();    break;
    case 3: m_warehouseTab->reload(); break;
    case 4: m_reportsTab->reload();   break;
    }
    statusBar()->showMessage("Обновлено: " +
                                 QDateTime::currentDateTime().toString("HH:mm:ss"), 3000);
}

void DirectorMainWindow::onLogout()
{
    if (QMessageBox::question(this, "Выход",
                              "Выйти из системы?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    m_refreshTimer->stop();
    Session::instance().clear();

    auto* loginWin = new LoginWindow();
    loginWin->show();
    close();
}