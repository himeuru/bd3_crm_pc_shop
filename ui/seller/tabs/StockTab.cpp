#include "StockTab.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

StockTab::StockTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void StockTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* btnRow = new QHBoxLayout();
    auto* lblHint = new QLabel(
        "Товары отсортированы по убыванию остатка — заканчивающиеся сверху", this);
    lblHint->setStyleSheet("color: #6c7086; font-size: 12px;");
    auto* btnRefresh = new QPushButton("Обновить", this);
    btnRow->addWidget(lblHint);
    btnRow->addStretch();
    btnRow->addWidget(btnRefresh);

    m_table = new SqlTableWidget(this);
    m_table->setTitle("Товары на микроскладе");
    m_table->setSearchable(true);
    m_table->setMinimumHeight(200);

    layout->addLayout(btnRow);
    layout->addWidget(m_table, 1);

    connect(btnRefresh, &QPushButton::clicked, this, &StockTab::reload);
}

void StockTab::reload()
{
    m_table->setQuery(
        QString(
            "SELECT p.name AS \"Товар\", p.category AS \"Категория\", "
            "  ws.quantity AS \"Остаток\", "
            "  p.cost_price AS \"Себестоимость\", "
            "  to_char(ws.updated_at, 'DD.MM.YYYY HH24:MI') AS \"Обновлено\" "
            "FROM warehouse_stock ws "
            "JOIN warehouses w ON w.id = ws.warehouse_id "
            "  AND w.shop_id = %1 AND w.type = 'micro' "
            "JOIN products p ON p.id = ws.product_id "
            "ORDER BY ws.quantity ASC, p.name"
        ).arg(Session::instance().shopId),
        {"Товар", "Категория", "Остаток", "Себестоимость", "Обновлено"}
    );
}
