#include "ClientMainWindow.h"
#include "tabs/CatalogTab.h"
#include "tabs/CartTab.h"
#include "tabs/OrderHistoryTab.h"
#include "tabs/ProfileTab.h"
#include "../../core/Session.h"
#include "../../ui/LoginWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QWidget>
#include <QPushButton>
#include <QMessageBox>

ClientMainWindow::ClientMainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("CRM PC Shop — Личный кабинет");
    setMinimumSize(800, 600);
    resize(1000, 680);
    buildUi();
}

void ClientMainWindow::buildUi()
{
    m_tabs = new QTabWidget(this);
    m_tabs->addTab(new CatalogTab(this),      "🛍  Каталог");
    m_tabs->addTab(new CartTab(this),         "🛒  Корзина");
    m_tabs->addTab(new OrderHistoryTab(this), "📋  Мои заказы");
    m_tabs->addTab(new ProfileTab(this),      "👤  Профиль");

    auto* header  = new QWidget(this);
    header->setStyleSheet("background: #181825; border-bottom: 1px solid #313244;");
    auto* hLayout = new QHBoxLayout(header);
    hLayout->setContentsMargins(16, 8, 16, 8);

    auto* nameLabel = new QLabel(
        QString("⚙  CRM PC Shop  |  %1").arg(Session::instance().fullName), header);
    nameLabel->setStyleSheet("color: #cdd6f4; font-size: 13px;");

    auto* btnLogout = new QPushButton("Выйти", header);
    btnLogout->setObjectName("btnDanger");
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

    connect(btnLogout, &QPushButton::clicked, this, &ClientMainWindow::onLogout);
}

void ClientMainWindow::onLogout()
{
    if (QMessageBox::question(this, "Выход", "Выйти из личного кабинета?",
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    Session::instance().clear();
    (new LoginWindow())->show();
    close();
}
