#include "SetupDialog.h"
#include "../core/DatabaseManager.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QTimer>

// Экранирует строку для безопасной вставки в SQL без prepared statements.
// Заменяет ' на '' (стандарт SQL) и убирает \0.
static QString esc(const QString& s)
{
    QString r = s;
    r.replace("'", "''");
    r.replace(QChar('\0'), "");
    return r;
}

// ─── Статическая проверка ─────────────────────────────────────────────────

bool SetupDialog::isFirstRun()
{
    QSqlQuery q(DatabaseManager::instance().db());
    q.exec("SELECT COUNT(*) FROM employees");
    if (q.next()) return q.value(0).toInt() == 0;
    return true;
}

// ─── Конструктор ──────────────────────────────────────────────────────────

SetupDialog::SetupDialog(QWidget* parent) : QDialog(parent)
{
    setWindowTitle("Первоначальная настройка — CRM PC Shop");
    setMinimumWidth(460);
    setModal(true);
    buildUi();
    adjustSize();
}

// ─── UI ───────────────────────────────────────────────────────────────────

void SetupDialog::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 24);
    root->setSpacing(10);

    auto* iconLabel = new QLabel("🏪", this);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setStyleSheet("font-size: 36px; background: transparent;");

    auto* titleLabel = new QLabel("Первоначальная настройка", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: #89b4fa; background: transparent;");

    auto* subLabel = new QLabel(
        "В базе данных ещё нет пользователей.\n"
        "Создайте первый магазин и аккаунт директора.", this);
    subLabel->setAlignment(Qt::AlignCenter);
    subLabel->setWordWrap(true);
    subLabel->setStyleSheet("color: #6c7086; font-size: 12px; background: transparent;");

    const QString fieldStyle =
        "QLineEdit, QSpinBox {"
        "  background: #313244; color: #cdd6f4;"
        "  border: 1px solid #45475a; border-radius: 6px;"
        "  padding: 0 10px; font-size: 13px; min-height: 36px;"
        "}"
        "QLineEdit:focus, QSpinBox:focus { border: 1px solid #89b4fa; }"
        "QSpinBox::up-button, QSpinBox::down-button { background: #45475a; border: none; width: 18px; }";

    // ── Группа: магазин ──
    auto* shopGroup = new QGroupBox("Первый магазин", this);
    auto* shopForm  = new QFormLayout(shopGroup);
    shopForm->setSpacing(8);
    shopForm->setContentsMargins(14, 16, 14, 14);
    shopForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_shopName = new QLineEdit(shopGroup);
    m_shopName->setPlaceholderText("PC Shop Москва");
    m_shopName->setFixedHeight(38);
    m_shopName->setStyleSheet(fieldStyle);

    m_shopAddress = new QLineEdit(shopGroup);
    m_shopAddress->setPlaceholderText("ул. Тверская, 1");
    m_shopAddress->setFixedHeight(38);
    m_shopAddress->setStyleSheet(fieldStyle);

    shopForm->addRow("Название:", m_shopName);
    shopForm->addRow("Адрес:",    m_shopAddress);

    // ── Группа: директор ──
    auto* dirGroup = new QGroupBox("Аккаунт директора", this);
    auto* dirForm  = new QFormLayout(dirGroup);
    dirForm->setSpacing(8);
    dirForm->setContentsMargins(14, 16, 14, 14);
    dirForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    m_fullName = new QLineEdit(dirGroup);
    m_fullName->setPlaceholderText("Иванов Иван Иванович");
    m_fullName->setFixedHeight(38);
    m_fullName->setStyleSheet(fieldStyle);

    m_login = new QLineEdit(dirGroup);
    m_login->setPlaceholderText("director");
    m_login->setFixedHeight(38);
    m_login->setStyleSheet(fieldStyle);

    m_password = new QLineEdit(dirGroup);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText("Минимум 6 символов");
    m_password->setFixedHeight(38);
    m_password->setStyleSheet(fieldStyle);

    m_passwordConfirm = new QLineEdit(dirGroup);
    m_passwordConfirm->setEchoMode(QLineEdit::Password);
    m_passwordConfirm->setPlaceholderText("Повторите пароль");
    m_passwordConfirm->setFixedHeight(38);
    m_passwordConfirm->setStyleSheet(fieldStyle);

    m_salary = new QSpinBox(dirGroup);
    m_salary->setRange(30000, 1000000);
    m_salary->setValue(100000);
    m_salary->setSuffix(" ₽");
    m_salary->setSingleStep(5000);
    m_salary->setFixedHeight(38);
    m_salary->setStyleSheet(fieldStyle);

    dirForm->addRow("ФИО:",           m_fullName);
    dirForm->addRow("Логин:",         m_login);
    dirForm->addRow("Пароль:",        m_password);
    dirForm->addRow("Подтверждение:", m_passwordConfirm);
    dirForm->addRow("Оклад:",         m_salary);

    m_status = new QLabel("", this);
    m_status->setAlignment(Qt::AlignCenter);
    m_status->setWordWrap(true);
    m_status->setMinimumHeight(40);
    m_status->setStyleSheet("font-size: 12px; background: transparent;");

    m_btnCreate = new QPushButton("Создать и войти как директор", this);
    m_btnCreate->setFixedHeight(44);
    m_btnCreate->setStyleSheet(
        "QPushButton { background: #a6e3a1; color: #1e1e2e; border: none;"
        "  border-radius: 8px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover    { background: #94e2d5; }"
        "QPushButton:pressed  { background: #89dceb; }"
        "QPushButton:disabled { background: #45475a; color: #6c7086; }"
        );

    root->addWidget(iconLabel);
    root->addWidget(titleLabel);
    root->addWidget(subLabel);
    root->addSpacing(4);
    root->addWidget(shopGroup);
    root->addWidget(dirGroup);
    root->addWidget(m_status);
    root->addWidget(m_btnCreate);

    connect(m_btnCreate,       &QPushButton::clicked,     this, &SetupDialog::onCreateClicked);
    connect(m_passwordConfirm, &QLineEdit::returnPressed, this, &SetupDialog::onCreateClicked);
}

// ─── Вспомогательные ──────────────────────────────────────────────────────

void SetupDialog::setStatus(const QString& msg, bool ok)
{
    m_status->setText(msg);
    m_status->setStyleSheet(
        QString("font-size: 12px; background: transparent; color: %1;")
            .arg(ok ? "#a6e3a1" : "#f38ba8"));
}

bool SetupDialog::validate()
{
    if (m_shopName->text().trimmed().isEmpty()) {
        setStatus("✗ Введите название магазина", false);
        m_shopName->setFocus(); return false;
    }
    if (m_fullName->text().trimmed().isEmpty()) {
        setStatus("✗ Введите ФИО директора", false);
        m_fullName->setFocus(); return false;
    }
    if (m_login->text().trimmed().length() < 3) {
        setStatus("✗ Логин должен быть не менее 3 символов", false);
        m_login->setFocus(); return false;
    }
    if (m_password->text().length() < 6) {
        setStatus("✗ Пароль должен быть не менее 6 символов", false);
        m_password->setFocus(); return false;
    }
    if (m_password->text() != m_passwordConfirm->text()) {
        setStatus("✗ Пароли не совпадают", false);
        m_passwordConfirm->setFocus();
        m_passwordConfirm->clear(); return false;
    }
    return true;
}

// ─── Создание (без prepare/bindValue — прямые exec со строками) ───────────

void SetupDialog::onCreateClicked()
{
    if (!validate()) return;

    m_btnCreate->setEnabled(false);
    m_btnCreate->setText("Создаём...");
    setStatus("", true);

    // Все значения экранируем через esc() — безопасно от SQL-инъекций
    const QString shopName    = esc(m_shopName->text().trimmed());
    const QString shopAddr    = esc(m_shopAddress->text().trimmed());
    const QString dirName     = esc(m_fullName->text().trimmed());
    const QString dirLogin    = esc(m_login->text().trimmed());
    const QString dirPassword = esc(m_password->text());
    const int     dirSalary   = m_salary->value();

    auto& db = DatabaseManager::instance();

    // ── Шаг 1: магазин ──
    QSqlQuery q1(db.db());
    if (!q1.exec(QString(
                     "INSERT INTO shops (name, address) VALUES ('%1', '%2') RETURNING id")
                     .arg(shopName, shopAddr))
        || !q1.next())
    {
        setStatus("✗ Ошибка создания магазина: " + q1.lastError().text(), false);
        m_btnCreate->setEnabled(true);
        m_btnCreate->setText("Создать и войти как директор");
        return;
    }
    int shopId = q1.value(0).toInt();

    // ── Шаг 2: главный склад (если нет) ──
    QSqlQuery q2(db.db());
    q2.exec(
        "INSERT INTO warehouses (name, type, shop_id) "
        "VALUES ('Центральный склад', 'main', NULL) "
        "ON CONFLICT DO NOTHING");

    // ── Шаг 3: микросклад ──
    QSqlQuery q3(db.db());
    if (!q3.exec(QString(
                     "INSERT INTO warehouses (name, type, shop_id) "
                     "VALUES ('Микросклад %1', 'micro', %2)")
                     .arg(shopName).arg(shopId)))
    {
        setStatus("✗ Ошибка создания склада: " + q3.lastError().text(), false);
        // Откатываем магазин
        QSqlQuery qr(db.db());
        qr.exec(QString("DELETE FROM shops WHERE id = %1").arg(shopId));
        m_btnCreate->setEnabled(true);
        m_btnCreate->setText("Создать и войти как директор");
        return;
    }

    // ── Шаг 4: директор через hire_employee() ──
    QSqlQuery q4(db.db());
    if (!q4.exec(QString(
                     "SELECT hire_employee(%1, '%2', 'director', %3, 0.10, '%4', '%5')")
                     .arg(shopId)
                     .arg(dirName)
                     .arg(dirSalary)
                     .arg(dirLogin)
                     .arg(dirPassword)))
    {
        QString err = q4.lastError().text();
        if (err.contains("idx_employees_login") || err.contains("unique"))
            setStatus("✗ Такой логин уже занят", false);
        else if (err.contains("DIRECTOR_EXISTS"))
            setStatus("✗ В этом магазине уже есть директор", false);
        else
            setStatus("✗ Ошибка создания директора: " + err, false);

        // Откатываем магазин и склады
        QSqlQuery qr(db.db());
        qr.exec(QString("DELETE FROM warehouses WHERE shop_id = %1").arg(shopId));
        qr.exec(QString("DELETE FROM shops WHERE id = %1").arg(shopId));
        m_btnCreate->setEnabled(true);
        m_btnCreate->setText("Создать и войти как директор");
        return;
    }

    setStatus("✓ Готово! Войдите с вашим логином и паролем.", true);
    QTimer::singleShot(900, this, &QDialog::accept);
}
