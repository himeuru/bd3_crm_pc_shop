#include "CartTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QSqlQuery>
#include <QMessageBox>
#include <QClipboard>
#include <QApplication>
#include <QGroupBox>

static QString esc(const QString& s)
{
    QString r = s; r.remove(QChar('\0')); r.replace("'","''"); return r;
}

CartTab::CartTab(QWidget* parent) : QWidget(parent)
{
    QSqlQuery q(DatabaseManager::instance().db());
    if (q.exec(QString("SELECT discount_pct FROM clients WHERE id=%1")
                   .arg(Session::instance().userId)) && q.next())
        m_clientDiscount = q.value(0).toDouble();

    buildUi();
    onSearch();
}

void CartTab::buildUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12,12,12,12);
    root->setSpacing(8);

    // ── Баннер ────────────────────────────────────────────────────────
    auto* bannerRow = new QHBoxLayout;
    auto* bannerLabel = new QLabel(
        "Составьте список и покажите его продавцу — он оформит заказ с вашей скидкой");
    bannerLabel->setStyleSheet("color:#e2e4f0; font-size:13px;");

    m_discountInfo = new QLabel;
    m_discountInfo->setStyleSheet(
        "color:#73daca; font-weight:bold; padding:3px 10px;"
        " background:#0d2a22; border-radius:4px; border:1px solid #73daca;");
    m_discountInfo->setText(m_clientDiscount > 0.0001
                                ? QString("Скидка %1%").arg(m_clientDiscount*100,0,'f',1)
                                : "Без скидки");

    bannerRow->addWidget(bannerLabel,1);
    bannerRow->addWidget(m_discountInfo);

    // ── Строка поиска ─────────────────────────────────────────────────
    auto* searchRow = new QHBoxLayout;

    m_categoryFilter = new QComboBox;
    m_categoryFilter->setMinimumWidth(130);
    m_categoryFilter->addItem("Все категории","");
    for (const char* c : {"CPU","GPU","RAM","SSD","PSU","MB","COOL"})
        m_categoryFilter->addItem(QString(c), QString(c));

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Поиск по названию...");

    auto* btnSearch = new QPushButton("Найти");
    m_btnAdd        = new QPushButton("+ В список");
    m_btnAdd->setObjectName("btnSuccess");
    m_btnAdd->setEnabled(false);

    searchRow->addWidget(m_categoryFilter);
    searchRow->addWidget(m_searchEdit,1);
    searchRow->addWidget(btnSearch);
    searchRow->addWidget(m_btnAdd);

    // ── Сплиттер ──────────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Левая: каталог
    auto* leftPanel  = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0,0,0,0);
    leftLayout->setSpacing(6);

    m_catalogTable = new SqlTableWidget;
    m_catalogTable->setTitle("Каталог товаров");

    m_stockHint = new QLabel;
    m_stockHint->setStyleSheet("color:#9899b3; font-size:12px;");
    m_stockHint->hide();

    leftLayout->addLayout(searchRow);
    leftLayout->addWidget(m_catalogTable,1);
    leftLayout->addWidget(m_stockHint);

    // Правая: список желаний
    auto* rightPanel  = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0,0,0,0);
    rightLayout->setSpacing(6);

    auto* wishTitleRow = new QHBoxLayout;
    auto* wishTitle    = new QLabel("Список желаний");
    wishTitle->setStyleSheet("font-size:14px; font-weight:bold; color:#7aa2f7;");
    wishTitleRow->addWidget(wishTitle);
    wishTitleRow->addStretch();

    m_wishTable = new QTableWidget(0,4);
    m_wishTable->setHorizontalHeaderLabels({"Товар","Кат.","Цена","Кол-во"});
    m_wishTable->horizontalHeader()->setStretchLastSection(true);
    m_wishTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_wishTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_wishTable->verticalHeader()->hide();
    m_wishTable->setAlternatingRowColors(true);

    auto* wishBtnRow = new QHBoxLayout;
    auto* btnRemove  = new QPushButton("Удалить");
    btnRemove->setObjectName("btnDanger");
    auto* btnClear   = new QPushButton("Очистить");
    btnClear->setObjectName("btnDanger");
    wishBtnRow->addWidget(btnRemove);
    wishBtnRow->addWidget(btnClear);
    wishBtnRow->addStretch();

    m_totalLabel = new QLabel("Итого: ~0 руб");
    m_totalLabel->setStyleSheet(
        "font-size:16px; font-weight:bold; color:#7aa2f7; padding:6px 0;");
    auto* totalHint = new QLabel("Цены ориентировочные — уточняйте у продавца");
    totalHint->setStyleSheet("color:#9899b3; font-size:11px; font-style:italic;");

    auto* btnCopy = new QPushButton("Скопировать список");

    rightLayout->addLayout(wishTitleRow);
    rightLayout->addWidget(m_wishTable,1);
    rightLayout->addLayout(wishBtnRow);
    rightLayout->addWidget(m_totalLabel);
    rightLayout->addWidget(totalHint);
    rightLayout->addWidget(btnCopy);

    leftPanel->setMinimumWidth(350);
    rightPanel->setMinimumWidth(380);
    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({500, 500});
    splitter->setChildrenCollapsible(false);

    root->addLayout(bannerRow);
    root->addWidget(splitter,1);

    // ── Сигналы ───────────────────────────────────────────────────────
    connect(btnSearch,       &QPushButton::clicked,     this, &CartTab::onSearch);
    connect(m_searchEdit,    &QLineEdit::returnPressed, this, &CartTab::onSearch);
    connect(m_categoryFilter,QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CartTab::onSearch);
    connect(m_btnAdd,        &QPushButton::clicked,     this, &CartTab::onAddToWish);
    connect(btnRemove,       &QPushButton::clicked,     this, &CartTab::onRemoveFromWish);
    connect(btnClear,        &QPushButton::clicked,     this, &CartTab::onClearWish);
    connect(btnCopy,         &QPushButton::clicked,     this, &CartTab::onCopyList);

    connect(m_catalogTable, &SqlTableWidget::rowDoubleClicked,
            this,[this](int){ if(m_btnAdd->isEnabled()) onAddToWish(); });
    connect(m_catalogTable, &SqlTableWidget::rowSelected,
            this,[this](int row){
                m_btnAdd->setEnabled(row>=0);
                if(row>=0){ m_stockHint->setText("Двойной клик или + В список"); m_stockHint->show(); }
                else m_stockHint->hide();
            });
}

void CartTab::onSearch()
{
    QString text = esc(m_searchEdit->text().trimmed());
    QString cat  = esc(m_categoryFilter->currentData().toString());
    QString cc   = cat.isEmpty()  ? "" : QString("AND p.category='%1'").arg(cat);
    QString tc   = text.isEmpty() ? ""
                                : QString("AND lower(p.name) LIKE lower('%%%1%%')").arg(text);

    m_catalogTable->setQuery(
        QString("SELECT p.id,p.name,p.category,p.description,"
                " ROUND(calc_sale_price(p.cost_price,0.10,0.10,%1),0) AS \"Цена (руб)\""
                " FROM products p WHERE p.is_active %2 %3"
                " ORDER BY p.category,p.name")
            .arg(m_clientDiscount).arg(cc).arg(tc),
        {"ID","Товар","Категория","Описание","Цена (руб)"});
    m_btnAdd->setEnabled(false);
    m_stockHint->hide();
}

void CartTab::onAddToWish()
{
    int     pid   = m_catalogTable->currentData(0).toInt();
    QString name  = m_catalogTable->currentData(1).toString();
    QString cat   = m_catalogTable->currentData(2).toString();
    double  price = m_catalogTable->currentData(4).toDouble();
    if (pid <= 0) return;

    for (auto& it : m_wish) {
        if (it.productId == pid) { it.qty++; refreshWish(); return; }
    }
    m_wish.append({pid, name, cat, price, 1});
    refreshWish();
}

void CartTab::onRemoveFromWish()
{
    int row = m_wishTable->currentRow();
    if (row<0 || row>=m_wish.size()) return;
    m_wish.removeAt(row);
    refreshWish();
}

void CartTab::onClearWish()
{
    if (m_wish.isEmpty()) return;
    m_wish.clear();
    refreshWish();
}

void CartTab::refreshWish()
{
    m_wishTable->setRowCount(m_wish.size());
    double total = 0;
    for (int i=0; i<m_wish.size(); ++i) {
        const auto& it = m_wish[i];
        total += it.price * it.qty;
        m_wishTable->setItem(i,0, new QTableWidgetItem(it.name));
        m_wishTable->setItem(i,1, new QTableWidgetItem(it.category));
        auto* p = new QTableWidgetItem(QString::number(it.price,'f',0));
        p->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        m_wishTable->setItem(i,2,p);
        auto* q = new QTableWidgetItem(QString::number(it.qty));
        q->setTextAlignment(Qt::AlignCenter);
        m_wishTable->setItem(i,3,q);
    }
    m_totalLabel->setText(QString("Итого: ~%1 руб").arg(total,0,'f',0));
}

void CartTab::onCopyList()
{
    if (m_wish.isEmpty()) {
        QMessageBox::information(this,"Список пуст","Добавьте товары в список"); return;
    }
    QString text = QString("Список желаний — %1\n").arg(Session::instance().fullName);
    if (m_clientDiscount > 0.0001)
        text += QString("Скидка: %1%\n").arg(m_clientDiscount*100,0,'f',1);
    text += QString(60,'-') + "\n";
    double total = 0;
    for (const auto& it : m_wish) {
        double line = it.price * it.qty;
        total += line;
        text += QString("%1  x%2 = %3 руб\n").arg(it.name).arg(it.qty).arg(line,0,'f',0);
    }
    text += QString(60,'-') + "\n";
    text += QString("ИТОГО: ~%1 руб\n(цены ориентировочные)").arg(total,0,'f',0);
    QApplication::clipboard()->setText(text);
    QMessageBox::information(this,"Скопировано","Список скопирован в буфер обмена.\nПокажите его продавцу.");
}