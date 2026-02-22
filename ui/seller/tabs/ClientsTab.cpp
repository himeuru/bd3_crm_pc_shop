#include "ClientsTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QInputDialog>

ClientsTab::ClientsTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void ClientsTab::buildUi()
{
    auto* layout  = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* btnRow  = new QHBoxLayout();
    auto* btnReg  = new QPushButton("+ Зарегистрировать клиента", this);
    auto* btnDisc = new QPushButton("% Изменить скидку", this);
    btnReg->setObjectName("btnSuccess");
    btnRow->addWidget(btnReg);
    btnRow->addWidget(btnDisc);
    btnRow->addStretch();

    m_table = new SqlTableWidget(this);
    m_table->setTitle("Клиенты");
    m_table->setSearchable(true);

    layout->addLayout(btnRow);
    layout->addWidget(m_table);

    connect(btnReg,  &QPushButton::clicked, this, &ClientsTab::onRegisterClient);
    connect(btnDisc, &QPushButton::clicked, this, &ClientsTab::onSetDiscount);
}

void ClientsTab::reload()
{
    m_table->setQuery(
        "SELECT id, full_name AS \"Имя\", phone AS \"Телефон\", email AS \"Email\", "
        "  (discount_pct * 100)::numeric(5,1) || '%' AS \"Скидка\", "
        "  to_char(registered_at, 'DD.MM.YYYY') AS \"Зарегистрирован\" "
        "FROM clients ORDER BY full_name",
        {"ID", "Имя", "Телефон", "Email", "Скидка", "Зарегистрирован"}
    );
}

void ClientsTab::onRegisterClient()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Регистрация клиента");
    dlg.setMinimumWidth(360);
    auto* form      = new QFormLayout(&dlg);
    auto* editName  = new QLineEdit(&dlg);
    auto* editPhone = new QLineEdit(&dlg);
    editPhone->setPlaceholderText("+79001234567");
    auto* editEmail = new QLineEdit(&dlg);
    auto* editLogin = new QLineEdit(&dlg);
    auto* editPass  = new QLineEdit(&dlg);
    editPass->setEchoMode(QLineEdit::Password);
    auto* spinDisc  = new QDoubleSpinBox(&dlg);
    spinDisc->setRange(0, 50);
    spinDisc->setSuffix("%");
    spinDisc->setDecimals(1);
    auto* btnOk = new QPushButton("Зарегистрировать", &dlg);
    btnOk->setMinimumHeight(36);

    form->addRow("ФИО:",     editName);
    form->addRow("Телефон:", editPhone);
    form->addRow("Email:",   editEmail);
    form->addRow("Логин:",   editLogin);
    form->addRow("Пароль:",  editPass);
    form->addRow("Скидка:",  spinDisc);
    form->addRow("",         btnOk);

    connect(btnOk, &QPushButton::clicked, [&]() {
        if (editName->text().trimmed().isEmpty() || editLogin->text().trimmed().isEmpty()) {
            QMessageBox::warning(&dlg, "Ошибка", "ФИО и логин обязательны");
            return;
        }
        QSqlQuery q(DatabaseManager::instance().db());
        q.prepare(
            "INSERT INTO clients (full_name, phone, email, login, password_hash, discount_pct) "
            "VALUES (:name, :phone, :email, :login, crypt(:pass, gen_salt('bf', 12)), :disc)");
        q.bindValue(":name",  editName->text().trimmed());
        q.bindValue(":phone", editPhone->text().trimmed());
        q.bindValue(":email", editEmail->text().trimmed());
        q.bindValue(":login", editLogin->text().trimmed());
        q.bindValue(":pass",  editPass->text());
        q.bindValue(":disc",  spinDisc->value() / 100.0);
        if (!q.exec())
            QMessageBox::critical(&dlg, "Ошибка", q.lastError().text());
        else
            dlg.accept();
    });

    if (dlg.exec() == QDialog::Accepted)
        reload();
}

void ClientsTab::onSetDiscount()
{
    int clientId = m_table->currentData(0).toInt();
    if (clientId <= 0) {
        QMessageBox::warning(this, "Выбор", "Выберите клиента в таблице");
        return;
    }
    bool ok;
    double disc = QInputDialog::getDouble(this, "Скидка",
        "Новая скидка (0–50%):", 0, 0, 50, 1, &ok);
    if (!ok) return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE clients SET discount_pct = :disc WHERE id = :id");
    q.bindValue(":disc", disc / 100.0);
    q.bindValue(":id",   clientId);
    if (!q.exec())
        QMessageBox::critical(this, "Ошибка", q.lastError().text());
    else
        reload();
}
