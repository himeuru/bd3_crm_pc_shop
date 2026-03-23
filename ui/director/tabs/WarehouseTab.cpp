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
#include <QFrame>
#include <QDateTime>

WarehouseTab::WarehouseTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    reload();
}

void WarehouseTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    // ── Панель управления ─────────────────────────────────────────────
    auto* ctrlFrame = new QFrame(this);
    ctrlFrame->setFrameShape(QFrame::StyledPanel);
    ctrlFrame->setStyleSheet(
        "background:#1a1b26; border-radius:8px; border:1px solid #3b3d57;");
    auto* ctrlRow = new QHBoxLayout(ctrlFrame);
    ctrlRow->setContentsMargins(12, 10, 12, 10);
    ctrlRow->setSpacing(12);

    auto* lblFill = new QLabel("Пополнить микросклад до:", this);
    lblFill->setStyleSheet("font-weight: bold; color: #cdd6f4;");

    m_spinFillTo = new QSpinBox(this);
    m_spinFillTo->setRange(1, 500);
    m_spinFillTo->setValue(50);
    m_spinFillTo->setSuffix(" шт.");
    m_spinFillTo->setMinimumWidth(100);

    m_btnReplenish = new QPushButton("Пополнить", this);
    m_btnReplenish->setObjectName("btnSuccess");
    m_btnReplenish->setMinimumWidth(130);
    m_btnReplenish->setStyleSheet(
        "QPushButton { background:#73daca; color:#0f0f17; font-weight:bold;"
        " border-radius:6px; padding:7px 16px; }"
        "QPushButton:hover { background:#9ee8dc; }"
        "QPushButton:pressed { background:#50c9b8; }"
        );

    auto* btnRefresh = new QPushButton("Обновить", this);
    btnRefresh->setStyleSheet(
        "QPushButton { background:#7aa2f7; color:#0f0f17; font-weight:bold;"
        " border-radius:6px; padding:7px 16px; }"
        "QPushButton:hover { background:#a5bef8; }"
        "QPushButton:pressed { background:#5a82e0; }"
        );

    m_statusLabel = new QLabel("", this);
    m_statusLabel->setStyleSheet("color: #73daca; font-size: 12px;");
    m_statusLabel->hide();

    ctrlRow->addWidget(lblFill);
    ctrlRow->addWidget(m_spinFillTo);
    ctrlRow->addWidget(m_btnReplenish);
    ctrlRow->addWidget(btnRefresh);
    ctrlRow->addSpacing(16);
    ctrlRow->addWidget(m_statusLabel);
    ctrlRow->addStretch();

    // ── Сплиттер: главный склад | микросклад ──────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_mainTable = new SqlTableWidget(this);
    m_mainTable->setTitle("Главный склад");
    m_mainTable->setSearchable(true);

    m_microTable = new SqlTableWidget(this);
    m_microTable->setTitle("Микросклад магазина");
    m_microTable->setSearchable(true);

    m_mainTable->setMinimumSize(300, 200);
    m_microTable->setMinimumSize(300, 200);
    splitter->addWidget(m_mainTable);
    splitter->addWidget(m_microTable);
    splitter->setSizes({600, 600});          // явные начальные размеры, иначе панели схлопываются
    splitter->setChildrenCollapsible(false); // запретить схлопывание до нуля

    layout->addWidget(ctrlFrame);
    layout->addWidget(splitter, 1);

    connect(m_btnReplenish, &QPushButton::clicked, this, &WarehouseTab::onReplenish);
    connect(btnRefresh,     &QPushButton::clicked, this, &WarehouseTab::reload);
}

void WarehouseTab::reload()
{
    m_statusLabel->clear();
    m_statusLabel->hide();

    m_mainTable->setQuery(
        "SELECT p.name       AS \"Товар\","
        "  p.category        AS \"Категория\","
        "  ws.quantity        AS \"На главном складе\","
        "  p.cost_price       AS \"Себестоимость (руб)\""
        " FROM warehouse_stock ws"
        " JOIN warehouses w ON w.id = ws.warehouse_id AND w.type = 'main'"
        " JOIN products   p ON p.id = ws.product_id"
        " ORDER BY p.category, p.name",
        {"Товар", "Категория", "На главном складе", "Себестоимость"}
        );
    m_mainTable->setColumnMinWidth(3, 120);

    int shopId = Session::instance().shopId;
    m_microTable->setQuery(
        QString(
            "SELECT p.name       AS \"Товар\","
            "  p.category        AS \"Категория\","
            "  ws.quantity        AS \"На микроскладе\","
            "  p.cost_price       AS \"Себестоимость\","
            "  to_char(ws.updated_at, 'DD.MM HH24:MI') AS \"Обновлено\""
            " FROM warehouse_stock ws"
            " JOIN warehouses w ON w.id = ws.warehouse_id"
            "  AND w.shop_id = %1 AND w.type = 'micro'"
            " JOIN products   p ON p.id = ws.product_id"
            " ORDER BY ws.quantity ASC, p.name"
            ).arg(shopId),
        {"Товар", "Категория", "На складе", "Себестоимость", "Обновлено"}
        );
    m_microTable->setColumnMinWidth(3, 120);
}

void WarehouseTab::onReplenish()
{
    int fillTo = m_spinFillTo->value();
    int shopId = Session::instance().shopId;

    // Сначала подсчитать сколько позиций нужно пополнить (превью)
    QString previewSql = QString(
                             "SELECT COUNT(*) AS cnt, COALESCE(SUM(GREATEST(%1 - ws.quantity, 0)), 0) AS total_units"
                             " FROM warehouse_stock ws"
                             " JOIN warehouses w ON w.id = ws.warehouse_id"
                             "  AND w.shop_id = %2 AND w.type = 'micro'"
                             " WHERE ws.quantity < %1"
                             ).arg(fillTo).arg(shopId);

    QSqlQuery pq(DatabaseManager::instance().db());
    QString previewText;
    if (pq.exec(previewSql) && pq.next()) {
        int positions   = pq.value("cnt").toInt();
        int totalUnits  = pq.value("total_units").toInt();
        if (positions == 0) {
            QMessageBox::information(this, "Склад в норме",
                                     QString("Все товары уже имеют %1+ шт.\nПополнение не требуется.")
                                         .arg(fillTo));
            return;
        }
        previewText = QString(
                          "Будет пополнено %1 позиций товаров\n"
                          "Суммарно перенесено: ~%2 шт.\n\n"
                          "Продолжить?"
                          ).arg(positions).arg(totalUnits);
    } else {
        previewText = QString(
                          "Пополнить микросклад до %1 шт. каждого товара?\n\nПродолжить?")
                          .arg(fillTo);
    }

    if (QMessageBox::question(this, "Пополнение микросклада",
                              previewText,
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    // ИСПРАВЛЕНИЕ: используем exec() напрямую без prepare()/bindValue()
    QString sql = QString(
                      "SELECT replenish_micro_warehouse(%1, %2, %3)"
                      ).arg(shopId)
                      .arg(Session::instance().userId)
                      .arg(fillTo);

    QSqlQuery q(DatabaseManager::instance().db());
    if (!q.exec(sql)) {
        QString err = q.lastError().text();
        if (err.contains("not enough stock") || err.contains("quantity"))
            QMessageBox::warning(this, "Нет товара на главном складе",
                                 "На главном складе закончились некоторые товары.");
        else
            QMessageBox::critical(this, "Ошибка пополнения", err);
        return;
    }

    m_statusLabel->setText(
        QString("Пополнено до %1 шт. — %2")
            .arg(fillTo)
            .arg(QDateTime::currentDateTime().toString("HH:mm:ss")));
    m_statusLabel->show();

    reload();
    QMessageBox::information(this, "Готово",
                             QString("Микросклад пополнен до %1 шт. по каждому товару").arg(fillTo));
}