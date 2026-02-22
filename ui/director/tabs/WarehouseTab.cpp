#include "WarehouseTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>

WarehouseTab::WarehouseTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void WarehouseTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* topRow = new QHBoxLayout();
    auto* lblFill = new QLabel("Пополнить до:", this);
    m_spinFillTo = new QSpinBox(this);
    m_spinFillTo->setRange(1, 500);
    m_spinFillTo->setValue(50);
    m_btnReplenish = new QPushButton("⬇ Пополнить микросклад", this);
    m_btnReplenish->setObjectName("btnSuccess");
    topRow->addWidget(lblFill);
    topRow->addWidget(m_spinFillTo);
    topRow->addWidget(m_btnReplenish);
    topRow->addStretch();

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_mainTable = new SqlTableWidget(this);
    m_mainTable->setTitle("Главный склад");
    m_mainTable->setSearchable(true);

    m_microTable = new SqlTableWidget(this);
    m_microTable->setTitle("Микросклад магазина");
    m_microTable->setSearchable(true);

    splitter->addWidget(m_mainTable);
    splitter->addWidget(m_microTable);

    layout->addLayout(topRow);
    layout->addWidget(splitter);

    connect(m_btnReplenish, &QPushButton::clicked, this, &WarehouseTab::onReplenish);
}

void WarehouseTab::reload()
{
    m_mainTable->setQuery(
        "SELECT "
        "  p.name        AS \"Товар\", "
        "  p.category    AS \"Категория\", "
        "  ws.quantity   AS \"Кол-во на главном\", "
        "  p.cost_price  AS \"Себестоимость\" "
        "FROM warehouse_stock ws "
        "JOIN warehouses w ON w.id = ws.warehouse_id AND w.type = 'main' "
        "JOIN products   p ON p.id = ws.product_id "
        "ORDER BY p.category, p.name",
        {"Товар", "Категория", "Кол-во", "Себестоимость"}
    );

    int shopId = Session::instance().shopId;
    m_microTable->setQuery(
        QString(
            "SELECT "
            "  p.name        AS \"Товар\", "
            "  p.category    AS \"Категория\", "
            "  ws.quantity   AS \"Кол-во\", "
            "  p.cost_price  AS \"Себестоимость\", "
            "  to_char(ws.updated_at, 'DD.MM.YYYY HH24:MI') AS \"Обновлено\" "
            "FROM warehouse_stock ws "
            "JOIN warehouses w ON w.id = ws.warehouse_id "
            "  AND w.shop_id = %1 AND w.type = 'micro' "
            "JOIN products   p ON p.id = ws.product_id "
            "ORDER BY ws.quantity ASC, p.name"
        ).arg(shopId),
        {"Товар", "Категория", "Кол-во", "Себестоимость", "Обновлено"}
    );
}

void WarehouseTab::onReplenish()
{
    if (QMessageBox::question(this, "Пополнение склада",
        QString("Пополнить микросклад до %1 единиц каждого товара?")
            .arg(m_spinFillTo->value()),
        QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT replenish_micro_warehouse(:shop, :user, :fill)");
    q.bindValue(":shop", Session::instance().shopId);
    q.bindValue(":user", Session::instance().userId);
    q.bindValue(":fill", m_spinFillTo->value());

    if (!q.exec())
        QMessageBox::critical(this, "Ошибка", q.lastError().text());
    else {
        QMessageBox::information(this, "Готово", "Микросклад успешно пополнен");
        reload();
    }
}
