#include "LoginWindow.h"
#include "ConnectDialog.h"
#include "director/DirectorMainWindow.h"
#include "seller/SellerMainWindow.h"
#include "client/ClientMainWindow.h"
#include "../core/Session.h"
#include "../core/DatabaseManager.h"
#include "../core/AppConfig.h"
#include "../repositories/AuthRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QApplication>

LoginWindow::LoginWindow(QWidget* parent)
    : QWidget(parent)
{
    setWindowTitle("CRM PC Shop — Вход");
    setFixedSize(420, 560);
    buildUi();
}

void LoginWindow::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    // Используем objectName-селектор (#loginCard), а не QFrame{} —
    // QFrame{} каскадно распространяется на дочерние виджеты и
    // перекрывает color у QLineEdit, делая текст невидимым.
    auto* card = new QFrame(this);
    card->setObjectName("loginCard");
    card->setStyleSheet("#loginCard { background: #181825; }");

    auto* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(50, 44, 50, 36);
    cardLayout->setSpacing(10);

    auto* logoLabel = new QLabel("⚙", card);
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setStyleSheet("font-size: 44px; background: transparent;");

    auto* titleLabel = new QLabel("CRM PC Shop", card);
    titleLabel->setObjectName("labelTitle");
    titleLabel->setAlignment(Qt::AlignCenter);

    auto* subLabel = new QLabel("Система управления магазином", card);
    subLabel->setObjectName("labelSubtitle");
    subLabel->setAlignment(Qt::AlignCenter);

    auto* divider = new QFrame(card);
    divider->setObjectName("loginDivider");
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("#loginDivider { background: #313244; max-height: 1px; }");

    const QString fieldStyle =
        "QLineEdit {"
        "  background: #313244;"
        "  color: #cdd6f4;"
        "  border: 1px solid #45475a;"
        "  border-radius: 7px;"
        "  padding: 0 12px;"
        "  font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #89b4fa;"
        "}"
        "QLineEdit[echoMode='2'] {"
        "  lineedit-password-character: 9679;"
        "}";

    auto* loginLabel = new QLabel("Логин", card);
    loginLabel->setStyleSheet("font-size: 12px; color: #6c7086; background: transparent;");

    m_login = new QLineEdit(card);
    m_login->setPlaceholderText("Введите логин");
    m_login->setFixedHeight(48);
    m_login->setStyleSheet(fieldStyle);

    auto* passLabel = new QLabel("Пароль", card);
    passLabel->setStyleSheet("font-size: 12px; color: #6c7086; background: transparent;");

    auto* passRow = new QHBoxLayout();
    passRow->setSpacing(6);

    m_password = new QLineEdit(card);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText("Введите пароль");
    m_password->setFixedHeight(48);
    m_password->setStyleSheet(fieldStyle);

    m_btnTogglePass = new QPushButton("👁", card);
    m_btnTogglePass->setFixedSize(48, 48);
    m_btnTogglePass->setStyleSheet(
        "QPushButton {"
        "  background: #313244;"
        "  color: #cdd6f4;"
        "  border: 1px solid #45475a;"
        "  border-radius: 7px;"
        "  font-size: 18px;"
        "  padding: 0;"
        "}"
        "QPushButton:hover { background: #45475a; }"
        );

    passRow->addWidget(m_password);
    passRow->addWidget(m_btnTogglePass);

    m_errorLabel = new QLabel("", card);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setStyleSheet(
        "color: #f38ba8; font-size: 12px; min-height: 18px; background: transparent;");

    m_btnLogin = new QPushButton("Войти", card);
    m_btnLogin->setFixedHeight(46);
    m_btnLogin->setStyleSheet(
        "QPushButton {"
        "  background: #89b4fa;"
        "  color: #1e1e2e;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-size: 15px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background: #b4befe; }"
        "QPushButton:pressed { background: #74c7ec; }"
        );

    auto* settingsRow = new QHBoxLayout();
    auto* btnSettings = new QPushButton("⚙ Настройки подключения", card);
    btnSettings->setFlat(true);
    btnSettings->setStyleSheet(
        "QPushButton { color: #6c7086; font-size: 11px; "
        "text-decoration: underline; background: transparent; border: none; }"
        "QPushButton:hover { color: #89b4fa; }"
        );
    settingsRow->addStretch();
    settingsRow->addWidget(btnSettings);
    settingsRow->addStretch();

    m_connLabel = new QLabel(card);
    m_connLabel->setAlignment(Qt::AlignCenter);
    m_connLabel->setStyleSheet(
        "color: #45475a; font-size: 11px; background: transparent;");

    const auto& cfg = AppConfig::instance().db;
    m_connLabel->setText(QString("Подключено: %1@%2:%3/%4")
                             .arg(cfg.user, cfg.host)
                             .arg(cfg.port)
                             .arg(cfg.dbName));

    cardLayout->addWidget(logoLabel);
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(subLabel);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(divider);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(loginLabel);
    cardLayout->addWidget(m_login);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(passLabel);
    cardLayout->addLayout(passRow);
    cardLayout->addWidget(m_errorLabel);
    cardLayout->addSpacing(2);
    cardLayout->addWidget(m_btnLogin);
    cardLayout->addSpacing(4);
    cardLayout->addLayout(settingsRow);
    cardLayout->addStretch();
    cardLayout->addWidget(m_connLabel);

    root->addWidget(card);

    connect(m_btnLogin,      &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(btnSettings,     &QPushButton::clicked, this, &LoginWindow::onSettingsClicked);
    connect(m_btnTogglePass, &QPushButton::clicked, this, &LoginWindow::onPasswordToggle);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginWindow::onLoginClicked);
    connect(m_login,    &QLineEdit::returnPressed, [this]{ m_password->setFocus(); });
}

void LoginWindow::onLoginClicked()
{
    m_errorLabel->clear();
    const QString login = m_login->text().trimmed();
    const QString pass  = m_password->text();

    if (login.isEmpty() || pass.isEmpty()) {
        m_errorLabel->setText("Заполните все поля");
        return;
    }

    m_btnLogin->setEnabled(false);
    m_btnLogin->setText("Проверяем...");

    AuthRepository repo;
    auto res = repo.login(login, pass);

    m_btnLogin->setEnabled(true);
    m_btnLogin->setText("Войти");

    if (!res.ok) {
        m_errorLabel->setText("Неверный логин или пароль");
        m_password->clear();
        m_password->setFocus();
        return;
    }

    auto& s    = Session::instance();
    s.userId   = res.userId;
    s.shopId   = res.shopId;
    s.fullName = res.fullName;
    DatabaseManager::instance().setSessionUser(res.userId);

    openRoleWindow(res.role);
}

void LoginWindow::openRoleWindow(const QString& role)
{
    QWidget* win = nullptr;

    if (role == "director") {
        Session::instance().role = UserRole::Director;
        win = new DirectorMainWindow();
    } else if (role == "seller") {
        Session::instance().role = UserRole::Seller;
        win = new SellerMainWindow();
    } else {
        Session::instance().role = UserRole::Client;
        win = new ClientMainWindow();
    }

    win->show();
    this->close();
}

void LoginWindow::onSettingsClicked()
{
    ConnectDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        const auto& cfg = AppConfig::instance().db;
        m_connLabel->setText(QString("Подключено: %1@%2:%3/%4")
                                 .arg(cfg.user, cfg.host)
                                 .arg(cfg.port)
                                 .arg(cfg.dbName));
    }
}

void LoginWindow::onPasswordToggle()
{
    if (m_password->echoMode() == QLineEdit::Password) {
        m_password->setEchoMode(QLineEdit::Normal);
        m_btnTogglePass->setText("🙈");
    } else {
        m_password->setEchoMode(QLineEdit::Password);
        m_btnTogglePass->setText("👁");
    }
}
