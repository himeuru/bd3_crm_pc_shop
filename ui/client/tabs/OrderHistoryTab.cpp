#include "OrderHistoryTab.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QSplitter>

OrderHistoryTab::OrderHistoryTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void OrderHistoryTab::buildUi()
{
    auto* layout   = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* splitter = new QSplitter(Qt::Vertical, this);

    m_table = new SqlTableWidget(this);
    m_table->setTitle("Мои заказы");

    m_itemsTable = new SqlTableWidget(this);
    m_itemsTable->setTitle("Состав заказа");

    splitter->addWidget(m_table);
    splitter->addWidget(m_itemsTable);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter);

    connect(m_table, &SqlTableWidget::rowSelected, this, [this](int row) {
        if (row < 0) return;
        int orderId = m_table->currentData(0).toInt();
        m_itemsTable->setQuery(
            QString(
                "SELECT p.name AS \"Товар\", oi.product_category AS \"Категория\", "
                "  oi.quantity AS \"Кол-во\", oi.sale_price AS \"Цена\", "
                "  oi.sale_price * oi.quantity AS \"Итого\" "
                "FROM order_items oi "
                "JOIN products p ON p.id = oi.product_id "
                "WHERE oi.order_id = %1 "
                "ORDER BY p.name"
            ).arg(orderId),
            {"Товар", "Категория", "Кол-во", "Цена", "Итого"}
        );
    });
}

void OrderHistoryTab::reload()
{
    m_table->setQuery(
        QString(
            "SELECT "
            "  o.id AS \"#\", "
            "  to_char(o.created_at, 'DD.MM.YYYY HH24:MI') AS \"Дата\", "
            "  o.total_amount AS \"Сумма\", "
            "  (o.discount_pct * 100)::numeric(5,1) || '%' AS \"Скидка\", "
            "  CASE o.status "
            "    WHEN 'completed' THEN '✓ Выполнен' "
            "    WHEN 'cancelled' THEN '✗ Отменён' "
            "  END AS \"Статус\" "
            "FROM orders o "
            "WHERE o.client_id = %1 "
            "ORDER BY o.created_at DESC"
        ).arg(Session::instance().userId),
        {"#", "Дата", "Сумма", "Скидка", "Статус"}
    );
}
