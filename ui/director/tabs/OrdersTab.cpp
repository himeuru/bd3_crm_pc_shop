#include "OrdersTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QDate>

OrdersTab::OrdersTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void OrdersTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* filterRow = new QHBoxLayout();
    filterRow->setSpacing(10);

    auto* lblFrom = new QLabel("С:", this);
    m_dateFrom = new QDateEdit(QDate::currentDate().addMonths(-1), this);
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setDisplayFormat("dd.MM.yyyy");

    auto* lblTo = new QLabel("по:", this);
    m_dateTo = new QDateEdit(QDate::currentDate(), this);
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDisplayFormat("dd.MM.yyyy");

    m_filterStatus = new QComboBox(this);
    m_filterStatus->addItem("Все", "all");
    m_filterStatus->addItem("Выполненные", "completed");
    m_filterStatus->addItem("Отменённые", "cancelled");

    auto* btnFilter = new QPushButton("Показать", this);

    filterRow->addWidget(lblFrom);
    filterRow->addWidget(m_dateFrom);
    filterRow->addWidget(lblTo);
    filterRow->addWidget(m_dateTo);
    filterRow->addWidget(m_filterStatus);
    filterRow->addWidget(btnFilter);
    filterRow->addStretch();

    m_btnCancel = new QPushButton("✗ Отменить заказ", this);
    m_btnCancel->setObjectName("btnDanger");
    m_btnCancel->setEnabled(false);
    filterRow->addWidget(m_btnCancel);

    auto* splitter = new QSplitter(Qt::Vertical, this);

    m_table = new SqlTableWidget(this);
    m_table->setTitle("Заказы");
    m_table->setSearchable(true);

    m_itemsTable = new SqlTableWidget(this);
    m_itemsTable->setTitle("Состав выбранного заказа");

    splitter->addWidget(m_table);
    splitter->addWidget(m_itemsTable);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    layout->addLayout(filterRow);
    layout->addWidget(splitter);

    connect(btnFilter,  &QPushButton::clicked, this, &OrdersTab::onFilter);
    connect(m_btnCancel, &QPushButton::clicked, this, &OrdersTab::onCancel);
    connect(m_table, &SqlTableWidget::rowSelected, this, &OrdersTab::onSelectionChanged);
}

void OrdersTab::reload()
{
    onFilter();
}

void OrdersTab::onFilter()
{
    int shopId = Session::instance().shopId;
    QString statusFilter = m_filterStatus->currentData().toString();
    QString statusClause = (statusFilter == "all")
        ? ""
        : QString("AND o.status = '%1'").arg(statusFilter);

    m_table->setQuery(
        QString(
            "SELECT "
            "  o.id             AS \"ID\", "
            "  to_char(o.created_at, 'DD.MM.YYYY HH24:MI') AS \"Дата\", "
            "  c.full_name      AS \"Клиент\", "
            "  e.full_name      AS \"Продавец\", "
            "  o.total_amount   AS \"Сумма\", "
            "  (o.discount_pct * 100)::numeric(5,1) || '%' AS \"Скидка\", "
            "  CASE o.status WHEN 'completed' THEN '✓ Выполнен' "
            "                WHEN 'cancelled' THEN '✗ Отменён' END AS \"Статус\" "
            "FROM orders o "
            "JOIN clients   c ON c.id = o.client_id "
            "JOIN employees e ON e.id = o.seller_id "
            "WHERE o.shop_id = %1 "
            "  AND o.created_at::date BETWEEN '%2' AND '%3' "
            "  %4 "
            "ORDER BY o.created_at DESC"
        ).arg(shopId)
         .arg(m_dateFrom->date().toString("yyyy-MM-dd"))
         .arg(m_dateTo->date().toString("yyyy-MM-dd"))
         .arg(statusClause),
        {"ID", "Дата", "Клиент", "Продавец", "Сумма", "Скидка", "Статус"}
    );
    m_itemsTable->setQuery("SELECT NULL LIMIT 0");
}

void OrdersTab::onSelectionChanged(int row)
{
    bool valid = row >= 0;
    m_btnCancel->setEnabled(valid);

    if (!valid) return;
    int orderId = m_table->currentData(0).toInt();

    m_itemsTable->setQuery(
        QString(
            "SELECT "
            "  p.name           AS \"Товар\", "
            "  oi.product_category AS \"Категория\", "
            "  oi.quantity      AS \"Кол-во\", "
            "  oi.cost_price    AS \"Себестоимость\", "
            "  oi.sale_price    AS \"Цена продажи\", "
            "  oi.sale_price * oi.quantity AS \"Итого\" "
            "FROM order_items oi "
            "JOIN products p ON p.id = oi.product_id "
            "WHERE oi.order_id = %1 "
            "ORDER BY p.name"
        ).arg(orderId),
        {"Товар", "Категория", "Кол-во", "Себестоимость", "Цена продажи", "Итого"}
    );
}

void OrdersTab::onCancel()
{
    int orderId = m_table->currentData(0).toInt();
    if (orderId <= 0) return;

    QString status = m_table->currentData(6).toString();
    if (status.contains("Отменён")) {
        QMessageBox::information(this, "Уже отменён", "Этот заказ уже отменён");
        return;
    }

    if (QMessageBox::question(this, "Отмена заказа",
        QString("Отменить заказ #%1? Товары вернутся на склад.").arg(orderId),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT cancel_order(:order, :director)");
    q.bindValue(":order",    orderId);
    q.bindValue(":director", Session::instance().userId);

    if (!q.exec()) {
        QString err = q.lastError().text();
        if (err.contains("ACCESS_DENIED"))
            QMessageBox::warning(this, "Отказ", "Только директор этого магазина может отменить заказ");
        else
            QMessageBox::critical(this, "Ошибка", err);
    } else {
        QMessageBox::information(this, "Готово", "Заказ отменён, товары возвращены на склад");
        reload();
    }
}
