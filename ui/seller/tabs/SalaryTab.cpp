#include "SalaryTab.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QSplitter>

SalaryTab::SalaryTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void SalaryTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* splitter = new QSplitter(Qt::Vertical, this);

    m_monthlyTable = new SqlTableWidget(this);
    m_monthlyTable->setTitle("Заработок по месяцам");

    m_ordersTable = new SqlTableWidget(this);
    m_ordersTable->setTitle("Мои заказы");

    splitter->addWidget(m_monthlyTable);
    splitter->addWidget(m_ordersTable);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter);
}

void SalaryTab::reload()
{
    int sellerId = Session::instance().userId;

    m_monthlyTable->setQuery(
        QString(
            "SELECT "
            "  to_char(DATE_TRUNC('month', o.created_at), 'MM.YYYY') AS \"Месяц\", "
            "  COUNT(o.id) AS \"Заказов\", "
            "  e.salary AS \"Оклад\", "
            "  COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0) AS \"Комиссия\", "
            "  e.salary + COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0) AS \"Итого\" "
            "FROM employees e "
            "LEFT JOIN orders o ON o.seller_id = e.id AND o.status = 'completed' "
            "LEFT JOIN order_items oi ON oi.order_id = o.id "
            "WHERE e.id = %1 "
            "GROUP BY DATE_TRUNC('month', o.created_at), e.salary "
            "ORDER BY 1 DESC NULLS LAST"
        ).arg(sellerId),
        {"Месяц", "Заказов", "Оклад", "Комиссия", "Итого"}
    );

    m_ordersTable->setQuery(
        QString(
            "SELECT "
            "  o.id AS \"#\", "
            "  to_char(o.created_at, 'DD.MM.YYYY HH24:MI') AS \"Дата\", "
            "  c.full_name AS \"Клиент\", "
            "  o.total_amount AS \"Сумма\", "
            "  CASE o.status WHEN 'completed' THEN 'Выполнен' ELSE 'Отменён' END AS \"Статус\" "
            "FROM orders o "
            "JOIN clients c ON c.id = o.client_id "
            "WHERE o.seller_id = %1 "
            "ORDER BY o.created_at DESC LIMIT 100"
        ).arg(sellerId),
        {"#", "Дата", "Клиент", "Сумма", "Статус"}
    );
}
