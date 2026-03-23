#include "OrdersTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
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
    m_dateFrom->setMinimumWidth(110);

    auto* lblTo = new QLabel("по:", this);
    m_dateTo = new QDateEdit(QDate::currentDate(), this);
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setDisplayFormat("dd.MM.yyyy");
    m_dateTo->setMinimumWidth(110);

    m_filterStatus = new QComboBox(this);
    m_filterStatus->addItem("Все статусы",   "all");
    m_filterStatus->addItem("Выполненные",   "completed");
    m_filterStatus->addItem("Отменённые",    "cancelled");

    auto* btnFilter = new QPushButton("Показать", this);

    filterRow->addWidget(lblFrom);
    filterRow->addWidget(m_dateFrom);
    filterRow->addWidget(lblTo);
    filterRow->addWidget(m_dateTo);
    filterRow->addWidget(m_filterStatus);
    filterRow->addWidget(btnFilter);
    filterRow->addStretch();

    m_btnCancel = new QPushButton("Отменить заказ", this);
    m_btnCancel->setObjectName("btnDanger");
    m_btnCancel->setEnabled(false);
    m_btnCancel->setToolTip("Выберите заказ в таблице");
    auto* btnRefreshOrders = new QPushButton("Обновить", this);
    filterRow->addWidget(m_btnCancel);
    filterRow->addWidget(btnRefreshOrders);

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
    layout->addWidget(splitter, 1);

    connect(btnFilter,        &QPushButton::clicked, this, &OrdersTab::onFilter);
    connect(m_btnCancel,      &QPushButton::clicked, this, &OrdersTab::onCancel);
    connect(btnRefreshOrders, &QPushButton::clicked, this, &OrdersTab::reload);
    connect(m_table, &SqlTableWidget::rowSelected, this, &OrdersTab::onSelectionChanged);
}

void OrdersTab::reload() { onFilter(); }

void OrdersTab::onFilter()
{
    int     shopId       = Session::instance().shopId;
    QString statusFilter = m_filterStatus->currentData().toString();
    QString statusClause = (statusFilter == "all")
                               ? "" : QString("AND o.status = '%1'").arg(statusFilter);

    m_table->setQuery(
        QString(
            "SELECT o.id,"
            "  to_char(o.created_at, 'DD.MM.YYYY HH24:MI') AS \"Дата\","
            "  c.full_name   AS \"Клиент\","
            "  e.full_name   AS \"Продавец\","
            "  o.total_amount AS \"Сумма (руб)\","
            "  (o.discount_pct * 100)::numeric(5,1) || '%%' AS \"Скидка\","
            "  CASE o.status"
            "    WHEN 'completed' THEN 'Выполнен'"
            "    WHEN 'cancelled' THEN 'Отменён'"
            "  END AS \"Статус\""
            " FROM orders o"
            " JOIN clients   c ON c.id = o.client_id"
            " JOIN employees e ON e.id = o.seller_id"
            " WHERE o.shop_id = %1"
            "  AND o.created_at::date BETWEEN '%2' AND '%3' %4"
            " ORDER BY o.created_at DESC"
            ).arg(shopId)
            .arg(m_dateFrom->date().toString("yyyy-MM-dd"))
            .arg(m_dateTo->date().toString("yyyy-MM-dd"))
            .arg(statusClause),
        {"ID", "Дата", "Клиент", "Продавец", "Сумма", "Скидка", "Статус"}
        );

    // Сброс нижней таблицы
    m_itemsTable->clear();
    m_btnCancel->setEnabled(false);
}

void OrdersTab::onSelectionChanged(int row)
{
    if (row < 0) {
        m_btnCancel->setEnabled(false);
        m_itemsTable->clear();
        return;
    }

    int     orderId = m_table->currentData(0).toInt();
    QString status  = m_table->currentData(6).toString();

    // Кнопка отмены — только для выполненных заказов
    bool canCancel = (status == "Выполнен");
    m_btnCancel->setEnabled(canCancel);
    m_btnCancel->setToolTip(canCancel ? "" : "Заказ уже отменён");

    m_itemsTable->setQuery(
        QString(
            "SELECT p.name          AS \"Товар\","
            "  oi.product_category   AS \"Категория\","
            "  oi.quantity            AS \"Кол-во\","
            "  oi.cost_price          AS \"Себестоим.\","
            "  oi.sale_price          AS \"Цена продажи\","
            "  oi.sale_price * oi.quantity AS \"Итого\","
            "  ROUND((oi.sale_price - oi.cost_price) * oi.quantity, 2) AS \"Прибыль\""
            " FROM order_items oi"
            " JOIN products p ON p.id = oi.product_id"
            " WHERE oi.order_id = %1"
            " ORDER BY p.name"
            ).arg(orderId),
        {"Товар", "Категория", "Кол-во", "Себестоим.", "Цена", "Итого", "Прибыль"}
        );
}

void OrdersTab::onCancel()
{
    int orderId = m_table->currentData(0).toInt();
    if (orderId <= 0) return;

    QString status = m_table->currentData(6).toString();
    if (status == "Отменён") {
        QMessageBox::information(this, "Уже отменён",
                                 "Этот заказ уже был отменён ранее");
        return;
    }

    if (QMessageBox::question(this, "Отмена заказа",
                              QString("Отменить заказ #%1?\nТовары будут возвращены на микросклад.")
                                  .arg(orderId),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    // ИСПРАВЛЕНИЕ: используем exec() напрямую без prepare()/bindValue()
    QString sql = QString("SELECT cancel_order(%1, %2)")
                      .arg(orderId)
                      .arg(Session::instance().userId);

    QSqlQuery q(DatabaseManager::instance().db());
    if (!q.exec(sql)) {
        QString err = q.lastError().text();
        if (err.contains("ACCESS_DENIED"))
            QMessageBox::warning(this, "Нет доступа",
                                 "Только директор этого магазина может отменять заказы");
        else if (err.contains("ORDER_ALREADY_CANCELLED"))
            QMessageBox::information(this, "Уже отменён",
                                     "Заказ был отменён параллельно другим пользователем");
        else
            QMessageBox::critical(this, "Ошибка", err);
        return;
    }

    QMessageBox::information(this, "Заказ отменён",
                             QString("Заказ #%1 отменён.\nТовары возвращены на микросклад.").arg(orderId));
    reload();
}