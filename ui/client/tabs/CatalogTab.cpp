#include "CatalogTab.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>

CatalogTab::CatalogTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
}

void CatalogTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);

    m_table = new SqlTableWidget(this);
    m_table->setTitle("Каталог товаров");
    m_table->setSearchable(true);
    layout->addWidget(m_table);

    m_table->setQuery(
        "SELECT "
        "  p.name        AS \"Товар\", "
        "  p.category    AS \"Категория\", "
        "  p.description AS \"Описание\", "
        "  p.cost_price * 2.0 AS \"Цена (ориент.)\" "
        "FROM products p "
        "WHERE p.is_active "
        "ORDER BY p.category, p.name",
        {"Товар", "Категория", "Описание", "Цена"}
    );
}
