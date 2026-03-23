#include "OrderHistoryTab.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>

OrderHistoryTab::OrderHistoryTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void OrderHistoryTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* splitter = new QSplitter(Qt::Vertical, this);

    m_table = new SqlTableWidget(this);
    m_table->setTitle("Мои заказы");
    m_table->setSearchable(true);

    m_itemsTable = new SqlTableWidget(this);
    m_itemsTable->setTitle("Состав заказа — кликните строку выше");

    m_table->setMinimumHeight(150);
    m_itemsTable->setMinimumHeight(100);
    splitter->addWidget(m_table);
    splitter->addWidget(m_itemsTable);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({400, 200});
    splitter->setChildrenCollapsible(false);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();
    auto* btnRefresh = new QPushButton("Обновить", this);
    btnRow->addWidget(btnRefresh);

    layout->addWidget(splitter, 1);
    layout->addLayout(btnRow);

    connect(m_table, &SqlTableWidget::rowSelected, this, [this](int row) {
        if (row < 0) {
            m_itemsTable->setTitle("Состав заказа — кликните строку выше");
            return;
        }
        int orderId = m_table->currentData(0).toInt();
        m_itemsTable->setTitle(QString("Состав заказа #%1").arg(orderId));
        m_itemsTable->setQuery(
            QString(
                "SELECT p.name AS \"Товар\","
                "  oi.product_category AS \"Категория\","
                "  oi.quantity AS \"Кол-во\","
                "  oi.sale_price AS \"Цена (руб)\","
                "  oi.sale_price * oi.quantity AS \"Итого (руб)\""
                " FROM order_items oi"
                " JOIN products p ON p.id = oi.product_id"
                " WHERE oi.order_id = %1"
                " ORDER BY p.name"
                ).arg(orderId),
            {"Товар", "Категория", "Кол-во", "Цена", "Итого"}
            );
    });

    connect(btnRefresh, &QPushButton::clicked, this, &OrderHistoryTab::reload);
}

void OrderHistoryTab::reload()
{
    m_table->setQuery(
        QString(
            "SELECT o.id AS \"#\","
            "  to_char(o.created_at, 'DD.MM.YYYY HH24:MI') AS \"Дата\","
            "  o.total_amount AS \"Сумма (руб)\","
            "  (o.discount_pct * 100)::numeric(4,1) || '%%' AS \"Скидка\","
            "  CASE o.status"
            "    WHEN 'completed' THEN 'Выполнен'"
            "    WHEN 'cancelled' THEN 'Отменён'"
            "  END AS \"Статус\""
            " FROM orders o"
            " WHERE o.client_id = %1"
            " ORDER BY o.created_at DESC"
            ).arg(Session::instance().userId),
        {"#", "Дата", "Сумма", "Скидка", "Статус"}
        );
    // Сброс нижней таблицы
    m_itemsTable->clear();
    m_itemsTable->setTitle("Состав заказа — кликните строку выше");
}