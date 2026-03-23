#include "ReportsTab.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDate>

ReportsTab::ReportsTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
}

void ReportsTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* filterRow = new QHBoxLayout();

    m_reportType = new QComboBox(this);
    m_reportType->addItem("Продажи по месяцам",     "monthly_sales");
    m_reportType->addItem("Зарплатная ведомость",    "salaries");
    m_reportType->addItem("Топ товаров",             "top_products");
    m_reportType->addItem("Топ категорий",           "top_categories");
    m_reportType->addItem("История изменений зарплат","salary_log");
    m_reportType->addItem("История пополнений склада","stock_log");

    auto* lblFrom = new QLabel("С:", this);
    m_dateFrom = new QDateEdit(QDate::currentDate().addMonths(-3), this);
    m_dateFrom->setCalendarPopup(true);
    m_dateFrom->setDisplayFormat("dd.MM.yyyy");
    m_dateFrom->setMinimumWidth(110);

    auto* lblTo = new QLabel("по:", this);
    m_dateTo = new QDateEdit(QDate::currentDate(), this);
    m_dateTo->setCalendarPopup(true);
    m_dateTo->setMinimumWidth(110);
    m_dateTo->setDisplayFormat("dd.MM.yyyy");

    auto* btnShow = new QPushButton("Сформировать", this);

    filterRow->addWidget(m_reportType);
    filterRow->addWidget(lblFrom);
    filterRow->addWidget(m_dateFrom);
    filterRow->addWidget(lblTo);
    filterRow->addWidget(m_dateTo);
    filterRow->addWidget(btnShow);
    filterRow->addStretch();

    m_table = new SqlTableWidget(this);
    m_table->setSearchable(true);

    layout->addLayout(filterRow);
    layout->addWidget(m_table, 1);

    connect(btnShow, &QPushButton::clicked, this, &ReportsTab::onShowReport);
}

void ReportsTab::reload()
{
    onShowReport();
}

void ReportsTab::onShowReport()
{
    int shopId  = Session::instance().shopId;
    QString rep = m_reportType->currentData().toString();
    QString from = m_dateFrom->date().toString("yyyy-MM-dd");
    QString to   = m_dateTo->date().toString("yyyy-MM-dd");

    m_table->setTitle(m_reportType->currentText());

    if (rep == "monthly_sales") {
        m_table->setQuery(
            QString(
                "SELECT "
                "  to_char(DATE_TRUNC('month', o.created_at), 'MM.YYYY') AS \"Месяц\", "
                "  COUNT(*)                  AS \"Заказов\", "
                "  SUM(o.total_amount)       AS \"Выручка\", "
                "  SUM(o.total_amount) - SUM(sub.cost) AS \"Валовая прибыль\" "
                "FROM orders o "
                "JOIN LATERAL ("
                "  SELECT SUM(oi.cost_price * oi.quantity) AS cost "
                "  FROM order_items oi WHERE oi.order_id = o.id"
                ") sub ON TRUE "
                "WHERE o.shop_id = %1 AND o.status = 'completed' "
                "  AND o.created_at::date BETWEEN '%2' AND '%3' "
                "GROUP BY DATE_TRUNC('month', o.created_at) "
                "ORDER BY 1 DESC"
                ).arg(shopId).arg(from).arg(to),
            {"Месяц", "Заказов", "Выручка", "Валовая прибыль"}
            );
    } else if (rep == "salaries") {
        m_table->setQuery(
            QString(
                "SELECT "
                "  e.full_name   AS \"Сотрудник\", "
                "  e.salary      AS \"Оклад\", "
                "  COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0) AS \"Комиссия\", "
                "  e.salary + COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0) AS \"Итого\" "
                "FROM employees e "
                "LEFT JOIN orders o ON o.seller_id = e.id AND o.status = 'completed' "
                "  AND o.created_at::date BETWEEN '%2' AND '%3' "
                "LEFT JOIN order_items oi ON oi.order_id = o.id "
                "WHERE e.shop_id = %1 AND e.role = 'seller' AND e.is_active "
                "GROUP BY e.id, e.full_name, e.salary "
                "ORDER BY \"Итого\" DESC"
                ).arg(shopId).arg(from).arg(to),
            {"Сотрудник", "Оклад", "Комиссия", "Итого"}
            );
    } else if (rep == "top_products") {
        m_table->setQuery(
            QString(
                "SELECT "
                "  p.name                              AS \"Товар\", "
                "  oi.product_category                 AS \"Категория\", "
                "  SUM(oi.quantity)                    AS \"Продано\", "
                "  SUM(oi.sale_price * oi.quantity)    AS \"Выручка\", "
                "  SUM((oi.sale_price - oi.cost_price) * oi.quantity) AS \"Прибыль\" "
                "FROM order_items oi "
                "JOIN products p ON p.id = oi.product_id "
                "JOIN orders   o ON o.id = oi.order_id AND o.status = 'completed' "
                "WHERE oi.shop_id = %1 "
                "  AND o.created_at::date BETWEEN '%2' AND '%3' "
                "GROUP BY p.name, oi.product_category "
                "ORDER BY \"Продано\" DESC "
                "LIMIT 20"
                ).arg(shopId).arg(from).arg(to),
            {"Товар", "Категория", "Продано", "Выручка", "Прибыль"}
            );
    } else if (rep == "top_categories") {
        m_table->setQuery(
            QString(
                "SELECT "
                "  oi.product_category                 AS \"Категория\", "
                "  SUM(oi.quantity)                    AS \"Продано\", "
                "  SUM(oi.sale_price * oi.quantity)    AS \"Выручка\" "
                "FROM order_items oi "
                "JOIN orders o ON o.id = oi.order_id AND o.status = 'completed' "
                "WHERE oi.shop_id = %1 "
                "  AND o.created_at::date BETWEEN '%2' AND '%3' "
                "GROUP BY oi.product_category "
                "ORDER BY \"Выручка\" DESC"
                ).arg(shopId).arg(from).arg(to),
            {"Категория", "Продано", "Выручка"}
            );
    } else if (rep == "salary_log") {
        m_table->setQuery(
            QString(
                "SELECT "
                "  to_char(sl.changed_at, 'DD.MM.YYYY HH24:MI') AS \"Дата\", "
                "  e.full_name     AS \"Сотрудник\", "
                "  sl.old_salary   AS \"Старый оклад\", "
                "  sl.new_salary   AS \"Новый оклад\", "
                "  sl.new_salary - sl.old_salary AS \"Изменение\", "
                "  d.full_name     AS \"Изменил\" "
                "FROM salary_log sl "
                "JOIN employees e ON e.id = sl.employee_id "
                "JOIN employees d ON d.id = sl.changed_by "
                "WHERE e.shop_id = %1 "
                "  AND sl.changed_at::date BETWEEN '%2' AND '%3' "
                "ORDER BY sl.changed_at DESC"
                ).arg(shopId).arg(from).arg(to),
            {"Дата", "Сотрудник", "Старый оклад", "Новый оклад", "Изменение", "Изменил"}
            );
    } else if (rep == "stock_log") {
        m_table->setQuery(
            QString(
                "SELECT "
                "  to_char(stl.transferred_at, 'DD.MM.YYYY HH24:MI') AS \"Дата\", "
                "  p.name          AS \"Товар\", "
                "  stl.quantity    AS \"Кол-во\", "
                "  fw.name         AS \"Откуда\", "
                "  tw.name         AS \"Куда\", "
                "  e.full_name     AS \"Инициатор\" "
                "FROM stock_transfer_log stl "
                "JOIN products    p  ON p.id  = stl.product_id "
                "JOIN warehouses  fw ON fw.id = stl.from_warehouse "
                "JOIN warehouses  tw ON tw.id = stl.to_warehouse "
                "JOIN employees   e  ON e.id  = stl.initiated_by "
                "WHERE tw.shop_id = %1 "
                "  AND stl.transferred_at::date BETWEEN '%2' AND '%3' "
                "ORDER BY stl.transferred_at DESC"
                ).arg(shopId).arg(from).arg(to),
            {"Дата", "Товар", "Кол-во", "Откуда", "Куда", "Инициатор"}
            );
    }
}