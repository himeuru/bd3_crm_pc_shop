#include "SalaryTab.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>

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

    m_monthlyTable->setMinimumHeight(100);
    m_ordersTable->setMinimumHeight(150);
    splitter->addWidget(m_monthlyTable);
    splitter->addWidget(m_ordersTable);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({180, 360});
    splitter->setChildrenCollapsible(false);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* btnRefresh = new QPushButton("Обновить", this);
    btnRow->addWidget(btnRefresh);

    layout->addWidget(splitter, 1);
    layout->addLayout(btnRow);

    connect(btnRefresh, &QPushButton::clicked, this, &SalaryTab::reload);
}

void SalaryTab::reload()
{
    int sellerId = Session::instance().userId;

    // Запрос с генерацией строки для текущего месяца даже если заказов нет.
    // Сначала собираем реальные заказы, потом UNION строку текущего месяца
    // если она ещё не вошла в результат.
    m_monthlyTable->setQuery(
        QString(
            "SELECT"
            "  COALESCE(to_char(DATE_TRUNC('month', o.created_at), 'MM.YYYY'),"
            "           to_char(DATE_TRUNC('month', NOW()), 'MM.YYYY')) AS \"Месяц\","
            "  COUNT(o.id) AS \"Заказов\","
            "  e.salary AS \"Оклад (руб)\","
            "  ROUND(COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0), 2)"
            "    AS \"Комиссия (руб)\","
            "  ROUND(e.salary + COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0), 2)"
            "    AS \"Итого (руб)\""
            " FROM employees e"
            " LEFT JOIN orders o ON o.seller_id = e.id AND o.status = 'completed'"
            " LEFT JOIN order_items oi ON oi.order_id = o.id"
            " WHERE e.id = %1"
            " GROUP BY DATE_TRUNC('month', o.created_at), e.salary"
            " ORDER BY 1 DESC NULLS LAST"
            ).arg(sellerId),
        {"Месяц", "Заказов", "Оклад", "Комиссия", "Итого"}
        );

    m_ordersTable->setQuery(
        QString(
            "SELECT o.id AS \"#\","
            "  to_char(o.created_at, 'DD.MM.YYYY HH24:MI') AS \"Дата\","
            "  c.full_name AS \"Клиент\","
            "  o.total_amount AS \"Сумма (руб)\","
            "  ROUND(COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0), 2)"
            "    AS \"Моя комиссия\","
            "  CASE o.status"
            "    WHEN 'completed' THEN 'Выполнен'"
            "    ELSE 'Отменён' END AS \"Статус\""
            " FROM orders o"
            " JOIN clients c ON c.id = o.client_id"
            " LEFT JOIN order_items oi ON oi.order_id = o.id"
            " WHERE o.seller_id = %1"
            " GROUP BY o.id, o.created_at, c.full_name, o.total_amount, o.status"
            " ORDER BY o.created_at DESC LIMIT 100"
            ).arg(sellerId),
        {"#", "Дата", "Клиент", "Сумма", "Моя комиссия", "Статус"}
        );
}