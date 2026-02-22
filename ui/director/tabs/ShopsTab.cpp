#include "ShopsTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

ShopsTab::ShopsTab(QWidget* parent)
    : QWidget(parent)
{
    buildUi();
    reload();
}

void ShopsTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    m_table = new SqlTableWidget(this);
    m_table->setTitle("Список магазинов");
    m_table->setSearchable(true);

    auto* btnRow = new QHBoxLayout();
    m_btnOpen  = new QPushButton("✓ Открыть магазин", this);
    m_btnClose = new QPushButton("✗ Закрыть магазин", this);
    m_btnOpen->setObjectName("btnSuccess");
    m_btnClose->setObjectName("btnDanger");
    m_btnOpen->setEnabled(false);
    m_btnClose->setEnabled(false);

    btnRow->addWidget(m_btnOpen);
    btnRow->addWidget(m_btnClose);
    btnRow->addStretch();

    layout->addWidget(m_table);
    layout->addLayout(btnRow);

    connect(m_table,   &SqlTableWidget::rowSelected,
            this,      &ShopsTab::onSelectionChanged);
    connect(m_btnOpen,  &QPushButton::clicked, this, &ShopsTab::onOpenShop);
    connect(m_btnClose, &QPushButton::clicked, this, &ShopsTab::onCloseShop);
}

void ShopsTab::reload()
{
    m_table->setQuery(
        "SELECT "
        "  s.id          AS \"ID\", "
        "  s.name        AS \"Магазин\", "
        "  s.address     AS \"Адрес\", "
        "  CASE WHEN s.is_open THEN 'Открыт' ELSE 'Закрыт' END AS \"Статус\", "
        "  (s.director_margin * 100)::numeric(5,2) || '%' AS \"Маржа директора\", "
        "  COALESCE(d.full_name, '—') AS \"Директор\", "
        "  COUNT(DISTINCT e.id) FILTER (WHERE e.role = 'seller' AND e.is_active) AS \"Продавцов\" "
        "FROM shops s "
        "LEFT JOIN employees d ON d.shop_id = s.id AND d.role = 'director' AND d.is_active "
        "LEFT JOIN employees e ON e.shop_id = s.id "
        "GROUP BY s.id, s.name, s.address, s.is_open, s.director_margin, d.full_name "
        "ORDER BY s.id",
        {"ID", "Магазин", "Адрес", "Статус", "Маржа директора", "Директор", "Продавцов"}
    );
}

int ShopsTab::selectedShopId() const
{
    return m_table->currentData(0).toInt();
}

void ShopsTab::onSelectionChanged(int row)
{
    bool valid = row >= 0;
    m_btnOpen->setEnabled(valid);
    m_btnClose->setEnabled(valid);
}

void ShopsTab::onOpenShop()
{
    int id = selectedShopId();
    if (id <= 0) return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE shops SET is_open = TRUE WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка", q.lastError().text());
        return;
    }
    reload();
    QMessageBox::information(this, "Готово", "Магазин открыт");
}

void ShopsTab::onCloseShop()
{
    int id = selectedShopId();
    if (id <= 0) return;

    if (QMessageBox::question(this, "Закрыть магазин",
        "Закрыть магазин? Продажи будут заблокированы.",
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("UPDATE shops SET is_open = FALSE WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        QMessageBox::critical(this, "Ошибка", q.lastError().text());
        return;
    }
    reload();
    QMessageBox::information(this, "Готово", "Магазин закрыт");
}
