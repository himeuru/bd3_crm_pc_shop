#include "CatalogTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSqlQuery>
#include <QLabel>
#include <QFrame>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>

static QString esc(const QString& s)
{
    QString r = s; r.remove(QChar('\0')); r.replace("'", "''"); return r;
}

CatalogTab::CatalogTab(QWidget* parent) : QWidget(parent)
{
    QSqlQuery q(DatabaseManager::instance().db());
    if (q.exec(QString("SELECT discount_pct FROM clients WHERE id = %1")
                   .arg(Session::instance().userId)) && q.next())
        m_clientDiscount = q.value(0).toDouble();

    buildUi();
    onSearch();  // загружаем сразу
}

void CatalogTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    // ── Баннер скидки ─────────────────────────────────────────────────
    auto* discFrame = new QFrame(this);
    discFrame->setStyleSheet(m_clientDiscount > 0.0001
                                 ? "background:#0d2a22; border-radius:6px; border:1px solid #73daca;"
                                 : "background:#13141f; border-radius:6px; border:1px solid #3b3d57;");
    auto* discRow = new QHBoxLayout(discFrame);
    discRow->setContentsMargins(14, 8, 14, 8);
    auto* discLabel = new QLabel(
        m_clientDiscount > 0.0001
            ? QString("Ваша персональная скидка %1% уже учтена в ценах")
                  .arg(m_clientDiscount * 100, 0, 'f', 1)
            : "Цены ориентировочные — уточняйте у продавца",
        this);
    discLabel->setStyleSheet(m_clientDiscount > 0.0001
                                 ? "color:#73daca; font-size:13px; font-weight:bold;"
                                 : "color:#9899b3; font-size:13px;");
    discRow->addWidget(discLabel);
    discRow->addStretch();

    // ── Строка поиска и фильтров ──────────────────────────────────────
    auto* searchRow = new QHBoxLayout();

    m_categoryFilter = new QComboBox(this);
    m_categoryFilter->addItem("Все категории", "");
    for (const char* c : {"CPU","GPU","RAM","SSD","PSU","MB","COOL"})
        m_categoryFilter->addItem(QString(c), QString(c));
    m_categoryFilter->setMinimumWidth(140);

    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Поиск по названию или описанию...");

    auto* btnSearch  = new QPushButton("Найти", this);
    auto* btnRefresh = new QPushButton("Обновить", this);

    searchRow->addWidget(m_categoryFilter);
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(btnSearch);
    searchRow->addWidget(btnRefresh);

    // ── Таблица ───────────────────────────────────────────────────────
    m_table = new SqlTableWidget(this);
    m_table->setTitle("Каталог товаров");
    m_table->setMinimumHeight(200);

    layout->addWidget(discFrame);
    layout->addLayout(searchRow);
    layout->addWidget(m_table, 1);

    connect(btnSearch,        &QPushButton::clicked,     this, &CatalogTab::onSearch);
    connect(m_searchEdit,     &QLineEdit::returnPressed, this, &CatalogTab::onSearch);
    connect(m_categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CatalogTab::onSearch);
    connect(btnRefresh, &QPushButton::clicked, this, &CatalogTab::onSearch);
}

void CatalogTab::onSearch()
{
    QString text = esc(m_searchEdit->text().trimmed());
    QString cat  = esc(m_categoryFilter->currentData().toString());

    QString catClause  = cat.isEmpty()  ? ""
                                      : QString("AND p.category = '%1'").arg(cat);
    QString textClause = text.isEmpty() ? ""
                                        : QString("AND (lower(p.name) LIKE lower('%%%1%%') "
                                                  "OR lower(COALESCE(p.description,'')) LIKE lower('%%%1%%'))").arg(text);

    // calc_sale_price: дефолтные 10% продавца + 10% директора + скидка клиента
    // Цены ориентировочные — точная зависит от конкретного магазина
    m_table->setQuery(
        QString(
            "SELECT p.name       AS \"Товар\","
            "  p.category        AS \"Категория\","
            "  p.description     AS \"Описание\","
            "  ROUND(calc_sale_price(p.cost_price, 0.10, 0.10, %1), 0)"
            "    AS \"Цена (руб)\""
            " FROM products p"
            " WHERE p.is_active %2 %3"
            " ORDER BY p.category, p.name"
            ).arg(m_clientDiscount).arg(catClause, textClause),
        {"Товар", "Категория", "Описание", "Цена (руб)"}
        );
}