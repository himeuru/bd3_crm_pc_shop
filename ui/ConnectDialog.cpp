#include "ConnectDialog.h"
#include "../core/DatabaseManager.h"
#include "../core/AppConfig.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

ConnectDialog::ConnectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Подключение к базе данных");
    setMinimumWidth(440);
    adjustSize();
    setModal(true);
    buildUi();
    loadFromConfig();
}

void ConnectDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 28, 28, 24);
    root->setSpacing(14);

    // Заголовок
    auto* titleRow = new QHBoxLayout();
    auto* iconLabel = new QLabel("🖥", this);
    iconLabel->setStyleSheet("font-size: 26px; background: transparent;");
    auto* titleLabel = new QLabel("CRM PC Shop", this);
    titleLabel->setStyleSheet(
        "font-size: 20px; font-weight: bold; color: #89b4fa; background: transparent;");
    titleRow->addStretch();
    titleRow->addWidget(iconLabel);
    titleRow->addSpacing(8);
    titleRow->addWidget(titleLabel);
    titleRow->addStretch();

    auto* subLabel = new QLabel("Настройка подключения к PostgreSQL", this);
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setStyleSheet("color: #6c7086; font-size: 12px; background: transparent;");

    // Группа с полями — используем objectName чтобы стиль не каскадировал
    // на дочерние QLineEdit и не перекрывал цвет текста
    auto* group = new QGroupBox("Параметры подключения", this);
    group->setObjectName("connectGroup");

    // Явный fieldStyle с color гарантирует видимость текста на любой теме
    const QString fieldStyle =
        "QLineEdit, QSpinBox {"
        "  background: #313244;"
        "  color: #cdd6f4;"
        "  border: 1px solid #45475a;"
        "  border-radius: 6px;"
        "  padding: 0 10px;"
        "  font-size: 13px;"
        "  min-height: 36px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus {"
        "  border: 1px solid #89b4fa;"
        "}"
        "QSpinBox::up-button, QSpinBox::down-button {"
        "  background: #45475a;"
        "  border: none;"
        "  width: 18px;"
        "}"
        "QSpinBox::up-arrow  { image: none; }"
        "QSpinBox::down-arrow { image: none; }";

    auto* form = new QFormLayout(group);
    form->setSpacing(10);
    form->setContentsMargins(16, 16, 16, 16);
    form->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_host = new QLineEdit(group);
    m_host->setPlaceholderText("localhost");
    m_host->setStyleSheet(fieldStyle);
    m_host->setFixedHeight(38);

    m_port = new QSpinBox(group);
    m_port->setRange(1, 65535);
    m_port->setValue(5432);
    m_port->setFixedHeight(38);
    m_port->setStyleSheet(fieldStyle);

    m_dbName = new QLineEdit(group);
    m_dbName->setPlaceholderText("crm_pc_shop");
    m_dbName->setStyleSheet(fieldStyle);
    m_dbName->setFixedHeight(38);

    m_user = new QLineEdit(group);
    m_user->setPlaceholderText("crm_app");
    m_user->setStyleSheet(fieldStyle);
    m_user->setFixedHeight(38);

    m_password = new QLineEdit(group);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText("••••••••");
    m_password->setStyleSheet(fieldStyle);
    m_password->setFixedHeight(38);

    form->addRow("Хост:",         m_host);
    form->addRow("Порт:",         m_port);
    form->addRow("База данных:",  m_dbName);
    form->addRow("Пользователь:", m_user);
    form->addRow("Пароль:",       m_password);

    // Статусная строка
    m_status = new QLabel("", this);
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setWordWrap(true);
    m_status->setMinimumHeight(20);
    m_status->setWordWrap(true);
    m_status->setStyleSheet("font-size: 12px; background: transparent;");

    // Кнопки
    auto* btnRow   = new QHBoxLayout();
    m_btnTest      = new QPushButton("Проверить", this);
    m_btnConnect   = new QPushButton("Подключиться", this);

    m_btnTest->setFixedHeight(38);
    m_btnConnect->setFixedHeight(38);
    m_btnConnect->setMinimumWidth(130);

    m_btnTest->setStyleSheet(
        "QPushButton {"
        "  background: transparent;"
        "  color: #89b4fa;"
        "  border: none;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { color: #b4befe; text-decoration: underline; }"
        );
    m_btnConnect->setStyleSheet(
        "QPushButton {"
        "  background: #89b4fa;"
        "  color: #1e1e2e;"
        "  border: none;"
        "  border-radius: 7px;"
        "  font-size: 13px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover  { background: #b4befe; }"
        "QPushButton:pressed{ background: #74c7ec; }"
        );

    btnRow->addWidget(m_btnTest);
    btnRow->addStretch();
    btnRow->addWidget(m_btnConnect);

    root->addLayout(titleRow);
    root->addWidget(subLabel);
    root->addWidget(group);
    root->addWidget(m_status);
    root->addLayout(btnRow);

    connect(m_btnTest,    &QPushButton::clicked, this, &ConnectDialog::onTestClicked);
    connect(m_btnConnect, &QPushButton::clicked, this, &ConnectDialog::onConnectClicked);
    connect(m_password,   &QLineEdit::returnPressed, this, &ConnectDialog::onConnectClicked);
}

void ConnectDialog::loadFromConfig()
{
    const auto& cfg = AppConfig::instance().db;
    m_host->setText(cfg.host);
    m_port->setValue(cfg.port);
    m_dbName->setText(cfg.dbName);
    m_user->setText(cfg.user);
    m_password->setText(cfg.password);
}

void ConnectDialog::saveToConfig()
{
    auto& cfg    = AppConfig::instance().db;
    cfg.host     = m_host->text().trimmed();
    cfg.port     = m_port->value();
    cfg.dbName   = m_dbName->text().trimmed();
    cfg.user     = m_user->text().trimmed();
    cfg.password = m_password->text();
    AppConfig::instance().save();
}

bool ConnectDialog::tryConnect()
{
    return DatabaseManager::instance().connect(
        m_host->text().trimmed(),
        m_port->value(),
        m_dbName->text().trimmed(),
        m_user->text().trimmed(),
        m_password->text()
        );
}

void ConnectDialog::setStatus(const QString& msg, bool ok)
{
    m_status->setText(msg);
    m_status->setStyleSheet(
        QString("font-size: 12px; background: transparent; color: %1;")
            .arg(ok ? "#a6e3a1" : "#f38ba8")
        );
}

void ConnectDialog::onTestClicked()
{
    m_btnTest->setEnabled(false);
    m_btnTest->setText("Проверяем...");
    setStatus("", true);

    if (tryConnect()) {
        setStatus("✓ Подключение успешно установлено", true);
        DatabaseManager::instance().disconnect();
    } else {
        setStatus("✗ " + DatabaseManager::instance().lastError(), false);
    }

    m_btnTest->setEnabled(true);
    m_btnTest->setText("Проверить");
}

void ConnectDialog::onConnectClicked()
{
    m_btnConnect->setEnabled(false);
    m_btnConnect->setText("Подключаемся...");
    setStatus("", true);

    if (tryConnect()) {
        saveToConfig();
        accept();
    } else {
        setStatus("✗ " + DatabaseManager::instance().lastError(), false);
        m_btnConnect->setEnabled(true);
        m_btnConnect->setText("Подключиться");
    }
}
