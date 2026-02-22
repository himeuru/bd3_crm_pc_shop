#include "StaffTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QInputDialog>

StaffTab::StaffTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void StaffTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    m_table = new SqlTableWidget(this);
    m_table->setTitle("Сотрудники магазина");
    m_table->setSearchable(true);

    auto* btnRow  = new QHBoxLayout();
    m_btnHire     = new QPushButton("+ Нанять", this);
    m_btnFire     = new QPushButton("Уволить", this);
    m_btnSalary   = new QPushButton("₽ Изменить зарплату", this);
    m_btnFire->setObjectName("btnDanger");
    m_btnFire->setEnabled(false);
    m_btnSalary->setEnabled(false);

    btnRow->addWidget(m_btnHire);
    btnRow->addWidget(m_btnFire);
    btnRow->addWidget(m_btnSalary);
    btnRow->addStretch();

    layout->addWidget(m_table);
    layout->addLayout(btnRow);

    connect(m_table,     &SqlTableWidget::rowSelected, this, &StaffTab::onSelectionChanged);
    connect(m_btnHire,   &QPushButton::clicked, this, &StaffTab::onHire);
    connect(m_btnFire,   &QPushButton::clicked, this, &StaffTab::onFire);
    connect(m_btnSalary, &QPushButton::clicked, this, &StaffTab::onChangeSalary);
}

void StaffTab::reload()
{
    int shopId = Session::instance().shopId;
    m_table->setQuery(
        QString(
            "SELECT "
            "  e.id           AS \"ID\", "
            "  e.full_name    AS \"Имя\", "
            "  e.role         AS \"Роль\", "
            "  e.salary       AS \"Оклад\", "
            "  (e.salary_pct * 100) || '%' AS \"Комиссия\", "
            "  CASE WHEN e.is_active THEN 'Активен' ELSE 'Уволен' END AS \"Статус\", "
            "  to_char(e.hired_at, 'DD.MM.YYYY') AS \"Принят\" "
            "FROM employees e "
            "WHERE e.shop_id = %1 "
            "ORDER BY e.role DESC, e.is_active DESC, e.full_name"
        ).arg(shopId),
        {"ID", "Имя", "Роль", "Оклад", "Комиссия", "Статус", "Принят"}
    );
}

void StaffTab::onSelectionChanged(int row)
{
    bool valid = row >= 0;
    m_btnFire->setEnabled(valid);
    m_btnSalary->setEnabled(valid);
}

void StaffTab::onHire()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Нанять сотрудника");
    dlg.setMinimumWidth(360);

    auto* form     = new QFormLayout(&dlg);
    auto* editName = new QLineEdit(&dlg);
    auto* comboRole = new QComboBox(&dlg);
    comboRole->addItem("Продавец", "seller");
    comboRole->addItem("Директор", "director");
    auto* editLogin  = new QLineEdit(&dlg);
    auto* editPass   = new QLineEdit(&dlg);
    editPass->setEchoMode(QLineEdit::Password);
    auto* spinSalary = new QDoubleSpinBox(&dlg);
    spinSalary->setRange(0, 1000000);
    spinSalary->setSuffix(" ₽");
    spinSalary->setValue(40000);

    auto* btnOk = new QPushButton("Нанять", &dlg);
    btnOk->setMinimumHeight(36);

    form->addRow("ФИО:",      editName);
    form->addRow("Роль:",     comboRole);
    form->addRow("Логин:",    editLogin);
    form->addRow("Пароль:",   editPass);
    form->addRow("Оклад:",    spinSalary);
    form->addRow("",          btnOk);

    connect(btnOk, &QPushButton::clicked, [&]() {
        QSqlQuery q(DatabaseManager::instance().db());
        q.prepare("SELECT hire_employee(:shop, :name, :role, :salary, 0.10, :login, :pass)");
        q.bindValue(":shop",   Session::instance().shopId);
        q.bindValue(":name",   editName->text().trimmed());
        q.bindValue(":role",   comboRole->currentData().toString());
        q.bindValue(":salary", spinSalary->value());
        q.bindValue(":login",  editLogin->text().trimmed());
        q.bindValue(":pass",   editPass->text());

        if (!q.exec()) {
            QString err = q.lastError().text();
            if (err.contains("DIRECTOR_EXISTS"))
                QMessageBox::warning(&dlg, "Ошибка", "В этом магазине уже есть директор");
            else
                QMessageBox::critical(&dlg, "Ошибка", err);
        } else {
            dlg.accept();
        }
    });

    if (dlg.exec() == QDialog::Accepted)
        reload();
}

void StaffTab::onFire()
{
    int empId = m_table->currentData(0).toInt();
    if (empId <= 0) return;

    QString name = m_table->currentData(1).toString();
    if (QMessageBox::question(this, "Уволить",
        QString("Уволить сотрудника <b>%1</b>?").arg(name),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT fire_employee(:emp, :dir)");
    q.bindValue(":emp", empId);
    q.bindValue(":dir", Session::instance().userId);

    if (!q.exec())
        QMessageBox::critical(this, "Ошибка", q.lastError().text());
    else
        reload();
}

void StaffTab::onChangeSalary()
{
    int empId = m_table->currentData(0).toInt();
    if (empId <= 0) return;

    bool ok;
    double newSalary = QInputDialog::getDouble(this, "Новый оклад",
        "Введите оклад (₽):", m_table->currentData(3).toDouble(), 0, 2000000, 2, &ok);
    if (!ok) return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT update_salary(:emp, :sal, :dir)");
    q.bindValue(":emp", empId);
    q.bindValue(":sal", newSalary);
    q.bindValue(":dir", Session::instance().userId);

    if (!q.exec())
        QMessageBox::critical(this, "Ошибка", q.lastError().text());
    else
        reload();
}
