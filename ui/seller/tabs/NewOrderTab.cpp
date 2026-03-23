#include "NewOrderTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QGroupBox>

static QString esc(const QString& s)
{
    QString r = s;
    r.remove(QChar('\0'));
    r.replace("'", "''");
    return r;
}

NewOrderTab::NewOrderTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
    loadProducts();
}

// ─────────────────────────────────────────────────────────────────────────────
//  UI
// ─────────────────────────────────────────────────────────────────────────────
void NewOrderTab::buildUi()
{
    // Все виджеты создаём БЕЗ явного parent — layout расставит сам.
    // Это предотвращает баг с "чёрными квадратами" (виджет с parent=this
    // рисовался как top-level поверх sub-panel без parent).

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // ── 1. Строка клиента ─────────────────────────────────────────────
    auto* clientGroup = new QGroupBox("Клиент");
    auto* clientRow   = new QHBoxLayout(clientGroup);
    clientRow->setSpacing(8);

    m_clientSearch    = new QLineEdit;
    m_clientSearch->setPlaceholderText("Логин или телефон...");
    m_clientSearch->setMinimumWidth(200);

    m_btnClientSearch = new QPushButton("Найти");

    m_clientLabel     = new QLabel("Не выбран");
    m_clientLabel->setStyleSheet("color:#f7768e; font-weight:bold;");

    m_discountLabel   = new QLabel;
    m_discountLabel->setStyleSheet("color:#73daca; font-weight:bold;");
    m_discountLabel->hide();

    auto* btnClear    = new QPushButton("Сбросить");
    btnClear->setObjectName("btnDanger");
    btnClear->setFixedSize(110, 34);

    clientRow->addWidget(m_clientSearch);
    clientRow->addWidget(m_btnClientSearch);
    clientRow->addWidget(m_clientLabel);
    clientRow->addWidget(m_discountLabel);
    clientRow->addStretch();
    clientRow->addWidget(btnClear);

    // ── 2. Строка поиска товаров ──────────────────────────────────────
    auto* searchRow = new QHBoxLayout;
    searchRow->setSpacing(6);

    m_categoryFilter = new QComboBox;
    m_categoryFilter->setMinimumWidth(130);
    m_categoryFilter->addItem("Все категории", "");
    for (const char* c : {"CPU","GPU","RAM","SSD","PSU","MB","COOL"})
        m_categoryFilter->addItem(QString(c), QString(c));

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Поиск по названию...");

    auto* btnSearch = new QPushButton("Найти");

    m_qtySpinner = new QSpinBox;
    m_qtySpinner->setRange(1, 9999);
    m_qtySpinner->setValue(1);
    m_qtySpinner->setPrefix("x ");
    m_qtySpinner->setFixedWidth(110);

    m_btnAddCart = new QPushButton("+ В корзину");
    m_btnAddCart->setObjectName("btnSuccess");
    m_btnAddCart->setEnabled(false);

    searchRow->addWidget(m_categoryFilter);
    searchRow->addWidget(m_searchEdit, 1);
    searchRow->addWidget(btnSearch);
    searchRow->addWidget(m_qtySpinner);
    searchRow->addWidget(m_btnAddCart);

    // ── 3. Сплиттер ───────────────────────────────────────────────────
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Левая панель — список товаров
    auto* leftPanel  = new QWidget;
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 4, 0, 0);
    leftLayout->setSpacing(6);

    m_productsTable = new SqlTableWidget;
    m_productsTable->setTitle("Товары на складе");

    m_stockHint = new QLabel;
    m_stockHint->setStyleSheet("color:#fab387; font-size:12px;");
    m_stockHint->hide();

    leftLayout->addLayout(searchRow);
    leftLayout->addWidget(m_productsTable, 1);
    leftLayout->addWidget(m_stockHint);

    // Правая панель — корзина
    auto* rightPanel  = new QWidget;
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 4, 0, 0);
    rightLayout->setSpacing(6);

    auto* cartTitleRow = new QHBoxLayout;
    auto* cartTitle    = new QLabel("Корзина");
    cartTitle->setStyleSheet("font-size:14px; font-weight:bold; color:#7aa2f7;");
    m_cartCountLabel   = new QLabel("0 позиций");
    m_cartCountLabel->setStyleSheet("color:#9899b3;");
    cartTitleRow->addWidget(cartTitle);
    cartTitleRow->addWidget(m_cartCountLabel);
    cartTitleRow->addStretch();

    m_cartTable = new QTableWidget(0, 5);
    m_cartTable->setHorizontalHeaderLabels({"Товар","Кат.","Цена","Кол-во","Итого"});
    m_cartTable->horizontalHeader()->setStretchLastSection(true);
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->verticalHeader()->hide();
    m_cartTable->setAlternatingRowColors(true);

    auto* cartBtnRow = new QHBoxLayout;
    auto* btnRemove  = new QPushButton("Удалить");
    btnRemove->setObjectName("btnDanger");
    auto* btnClearCart = new QPushButton("Очистить");
    btnClearCart->setObjectName("btnDanger");
    cartBtnRow->addWidget(btnRemove);
    cartBtnRow->addWidget(btnClearCart);
    cartBtnRow->addStretch();

    m_totalLabel = new QLabel("Итого: 0.00 руб");
    m_totalLabel->setStyleSheet(
        "font-size:16px; font-weight:bold; color:#7aa2f7; padding:6px 0;");

    m_discountSavingLabel = new QLabel;
    m_discountSavingLabel->setStyleSheet("color:#73daca; font-size:12px;");
    m_discountSavingLabel->hide();

    m_btnPlaceOrder = new QPushButton("Оформить заказ");
    m_btnPlaceOrder->setMinimumHeight(40);
    m_btnPlaceOrder->setEnabled(false);
    m_btnPlaceOrder->setObjectName("btnSuccess");

    rightLayout->addLayout(cartTitleRow);
    rightLayout->addWidget(m_cartTable, 1);
    rightLayout->addLayout(cartBtnRow);
    rightLayout->addWidget(m_totalLabel);
    rightLayout->addWidget(m_discountSavingLabel);
    rightLayout->addWidget(m_btnPlaceOrder);

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({700, 450});
    splitter->setChildrenCollapsible(false);

    // ── Собираем root layout ──────────────────────────────────────────
    root->addWidget(clientGroup);
    root->addWidget(splitter, 1);

    // ── Сигналы ───────────────────────────────────────────────────────
    connect(m_btnClientSearch, &QPushButton::clicked,     this, &NewOrderTab::onClientSearch);
    connect(m_clientSearch,    &QLineEdit::returnPressed, this, &NewOrderTab::onClientSearch);
    connect(btnClear,          &QPushButton::clicked,     this, &NewOrderTab::onClearClient);
    connect(btnSearch,         &QPushButton::clicked,     this, &NewOrderTab::onSearch);
    connect(m_searchEdit,      &QLineEdit::returnPressed, this, &NewOrderTab::onSearch);
    connect(m_categoryFilter,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &NewOrderTab::onSearch);
    connect(m_btnAddCart,      &QPushButton::clicked, this, &NewOrderTab::onAddToCart);
    connect(btnRemove,         &QPushButton::clicked, this, &NewOrderTab::onRemoveFromCart);
    connect(btnClearCart,      &QPushButton::clicked, this, &NewOrderTab::onClearCart);
    connect(m_btnPlaceOrder,   &QPushButton::clicked, this, &NewOrderTab::onPlaceOrder);

    // Двойной клик по товару — сразу в корзину
    connect(m_productsTable, &SqlTableWidget::rowDoubleClicked,
            this, [this](int) { if (m_btnAddCart->isEnabled()) onAddToCart(); });

    // Выбор строки — показать остаток, ограничить спиннер
    connect(m_productsTable, &SqlTableWidget::rowSelected,
            this, [this](int row) {
                bool valid = (row >= 0);
                m_btnAddCart->setEnabled(valid);
                if (valid) {
                    int stock = m_productsTable->currentData(3).toInt();
                    int inCart = 0;
                    int pid = m_productsTable->currentData(0).toInt();
                    for (const auto& it : m_cart)
                        if (it.productId == pid) inCart = it.qty;
                    int avail = qMax(stock - inCart, 0);
                    m_qtySpinner->setMaximum(avail > 0 ? avail : 1);
                    m_stockHint->setText(
                        QString("На складе: %1 шт.  |  В корзине: %2 шт.  |  Доступно: %3 шт.")
                            .arg(stock).arg(inCart).arg(avail));
                    m_stockHint->show();
                } else {
                    m_stockHint->hide();
                    m_qtySpinner->setMaximum(9999);
                }
            });
}

// ─────────────────────────────────────────────────────────────────────────────
//  Загрузка/поиск товаров
// ─────────────────────────────────────────────────────────────────────────────
void NewOrderTab::loadProducts()  { onSearch(); }
void NewOrderTab::reloadProducts(){ onSearch(); }

void NewOrderTab::onSearch()
{
    int     shopId   = Session::instance().shopId;
    int     sellerId = Session::instance().userId;
    QString text     = esc(m_searchEdit->text().trimmed());
    QString cat      = esc(m_categoryFilter->currentData().toString());

    QString catClause  = cat.isEmpty()  ? ""
                                      : QString("AND p.category = '%1'").arg(cat);
    QString textClause = text.isEmpty() ? ""
                                        : QString("AND (lower(p.name) LIKE lower('%%%1%%') "
                                                  "OR p.search_vec @@ plainto_tsquery('russian','%1'))").arg(text);

    QString sql = QString(
                      "SELECT p.id, p.name, p.category, ws.quantity,"
                      " calc_sale_price(p.cost_price,e.salary_pct,s.director_margin,%2) AS price"
                      " FROM warehouse_stock ws"
                      " JOIN warehouses w ON w.id=ws.warehouse_id"
                      "  AND w.shop_id=%1 AND w.type='micro'"
                      " JOIN products   p ON p.id=ws.product_id AND p.is_active"
                      " JOIN shops      s ON s.id=%1"
                      " JOIN employees  e ON e.id=%3"
                      " WHERE ws.quantity>0 %4 %5"
                      " ORDER BY p.category, p.name"
                      ).arg(shopId).arg(m_clientDiscount).arg(sellerId)
                      .arg(catClause).arg(textClause);

    m_productsTable->setQuery(sql,{"ID","Название","Категория","На складе","Цена (руб)"});
    m_btnAddCart->setEnabled(false);
    m_stockHint->hide();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Клиент
// ─────────────────────────────────────────────────────────────────────────────
void NewOrderTab::onClientSearch()
{
    QString q = esc(m_clientSearch->text().trimmed());
    if (q.isEmpty()) return;

    QSqlQuery sq(DatabaseManager::instance().db());
    if (!sq.exec(QString("SELECT id,full_name,discount_pct FROM clients"
                         " WHERE login='%1' OR phone='%1' LIMIT 1").arg(q))
        || !sq.next())
    {
        m_clientLabel->setText("Клиент не найден");
        m_clientLabel->setStyleSheet("color:#f7768e; font-weight:bold;");
        m_currentClientId = -1;
        m_clientDiscount  = 0.0;
        m_discountLabel->hide();
        updateOrderButton();
        return;
    }

    double prev       = m_clientDiscount;
    m_currentClientId = sq.value("id").toInt();
    m_clientName      = sq.value("full_name").toString();
    m_clientDiscount  = sq.value("discount_pct").toDouble();

    m_clientLabel->setText(m_clientName);
    m_clientLabel->setStyleSheet("color:#73daca; font-weight:bold;");
    m_discountLabel->setText(m_clientDiscount > 0.0001
                                 ? QString("Скидка %1%").arg(m_clientDiscount*100, 0,'f',1)
                                 : "Без скидки");
    m_discountLabel->show();

    if (qAbs(prev - m_clientDiscount) > 0.0001) { onSearch(); refreshCart(); }
    updateOrderButton();
}

void NewOrderTab::onClearClient()
{
    double prev = m_clientDiscount;
    m_currentClientId = -1;
    m_clientName.clear();
    m_clientDiscount  = 0.0;
    m_clientSearch->clear();
    m_clientLabel->setText("Не выбран");
    m_clientLabel->setStyleSheet("color:#f7768e; font-weight:bold;");
    m_discountLabel->hide();
    if (qAbs(prev) > 0.0001) { onSearch(); refreshCart(); }
    updateOrderButton();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Корзина
// ─────────────────────────────────────────────────────────────────────────────
void NewOrderTab::onAddToCart()
{
    int     pid     = m_productsTable->currentData(0).toInt();
    QString name    = m_productsTable->currentData(1).toString();
    QString cat     = m_productsTable->currentData(2).toString();
    int     stock   = m_productsTable->currentData(3).toInt();
    double  price   = m_productsTable->currentData(4).toDouble();
    int     qty     = m_qtySpinner->value();
    if (pid <= 0) return;

    int inCart = 0;
    for (const auto& it : m_cart)
        if (it.productId == pid) inCart = it.qty;

    if (inCart + qty > stock) {
        QMessageBox::warning(this, "Недостаточно на складе",
                             QString("На складе: %1 шт.\nВ корзине уже: %2 шт.\nМожно добавить: %3 шт.")
                                 .arg(stock).arg(inCart).arg(stock - inCart));
        return;
    }

    for (auto& it : m_cart) {
        if (it.productId == pid) { it.qty += qty; refreshCart(); return; }
    }
    m_cart.append({pid, name, cat, 0.0, price, qty});
    refreshCart();
}

void NewOrderTab::onRemoveFromCart()
{
    int row = m_cartTable->currentRow();
    if (row < 0 || row >= m_cart.size()) return;
    m_cart.removeAt(row);
    refreshCart();
}

void NewOrderTab::onClearCart()
{
    if (m_cart.isEmpty()) return;
    if (QMessageBox::question(this, "Очистить корзину",
                              "Удалить все товары?",
                              QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) return;
    m_cart.clear();
    refreshCart();
}

void NewOrderTab::refreshCart()
{
    m_cartTable->setRowCount(m_cart.size());
    double total = 0, totalFull = 0;

    for (int i = 0; i < m_cart.size(); ++i) {
        const auto& it = m_cart[i];
        double line = it.salePrice * it.qty;
        total += line;
        totalFull += (m_clientDiscount > 0.0001
                          ? it.salePrice / (1.0 - m_clientDiscount) : it.salePrice) * it.qty;

        m_cartTable->setItem(i,0, new QTableWidgetItem(it.name));
        m_cartTable->setItem(i,1, new QTableWidgetItem(it.category));
        auto* p = new QTableWidgetItem(QString::number(it.salePrice,'f',2));
        p->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        m_cartTable->setItem(i,2, p);
        auto* q = new QTableWidgetItem(QString::number(it.qty));
        q->setTextAlignment(Qt::AlignCenter);
        m_cartTable->setItem(i,3, q);
        auto* t = new QTableWidgetItem(QString::number(line,'f',2));
        t->setTextAlignment(Qt::AlignRight|Qt::AlignVCenter);
        m_cartTable->setItem(i,4, t);
    }

    m_cartCountLabel->setText(m_cart.isEmpty() ? "пусто"
                                               : QString("%1 позиц.").arg(m_cart.size()));
    m_totalLabel->setText(QString("Итого: %1 руб").arg(total, 0,'f',2));

    if (m_clientDiscount > 0.0001 && !m_cart.isEmpty()) {
        m_discountSavingLabel->setText(
            QString("Экономия %1%: %2 руб")
                .arg(m_clientDiscount*100,0,'f',1)
                .arg(totalFull - total, 0,'f',2));
        m_discountSavingLabel->show();
    } else {
        m_discountSavingLabel->hide();
    }
    updateOrderButton();
}

void NewOrderTab::updateOrderButton()
{
    bool ok = !m_cart.isEmpty() && m_currentClientId > 0;
    m_btnPlaceOrder->setEnabled(ok);
    if (!ok) {
        QStringList h;
        if (m_currentClientId <= 0) h << "выберите клиента";
        if (m_cart.isEmpty())       h << "добавьте товары";
        m_btnPlaceOrder->setToolTip(h.join(" и "));
    } else {
        m_btnPlaceOrder->setToolTip("");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Оформление заказа
// ─────────────────────────────────────────────────────────────────────────────
void NewOrderTab::onPlaceOrder()
{
    if (m_currentClientId <= 0) {
        QMessageBox::warning(this,"Клиент не выбран",
                             "Найдите клиента по логину или телефону"); return;
    }
    if (m_cart.isEmpty()) {
        QMessageBox::warning(this,"Корзина пуста",
                             "Добавьте хотя бы один товар"); return;
    }

    double total = 0;
    QString details;
    for (const auto& it : m_cart) {
        total += it.salePrice * it.qty;
        details += QString("  %1  x%2  = %3 руб\n")
                       .arg(it.name).arg(it.qty).arg(it.salePrice*it.qty,0,'f',2);
    }

    if (QMessageBox::question(this,"Подтверждение заказа",
                              QString("Клиент: %1\nСкидка: %2%\n\n%3\nИтого: %4 руб\n\nОформить?")
                                  .arg(m_clientName)
                                  .arg(m_clientDiscount*100,0,'f',1)
                                  .arg(details)
                                  .arg(total,0,'f',2),
                              QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) return;

    QJsonArray arr;
    for (const auto& it : m_cart) {
        QJsonObject o; o["product_id"] = it.productId; o["qty"] = it.qty;
        arr.append(o);
    }
    QString json = QString::fromUtf8(
        QJsonDocument(arr).toJson(QJsonDocument::Compact));
    json.replace("'","''");

    QString sql = QString("SELECT create_order(%1,%2,%3,'%4'::jsonb)")
                      .arg(Session::instance().shopId)
                      .arg(m_currentClientId)
                      .arg(Session::instance().userId)
                      .arg(json);

    QSqlQuery q(DatabaseManager::instance().db());
    if (!q.exec(sql) || !q.next()) {
        QString err = q.lastError().text();
        if (err.contains("INSUFFICIENT_STOCK"))
            QMessageBox::warning(this,"Нет товара",
                                 "Один из товаров закончился на складе.\n"
                                 "Попросите директора пополнить микросклад.");
        else if (err.contains("SHOP_CLOSED"))
            QMessageBox::warning(this,"Магазин закрыт",
                                 "Продажи заблокированы — магазин закрыт.");
        else
            QMessageBox::critical(this,"Ошибка",err);
        return;
    }

    int orderId = q.value(0).toInt();
    QMessageBox::information(this,"Заказ оформлен",
                             QString("Заказ #%1 создан!\nКлиент: %2\nСумма: %3 руб")
                                 .arg(orderId).arg(m_clientName).arg(total,0,'f',2));

    m_cart.clear();
    m_currentClientId = -1; m_clientName.clear(); m_clientDiscount = 0.0;
    m_clientSearch->clear();
    m_clientLabel->setText("не выбран");
    m_clientLabel->setStyleSheet("color:#f7768e; font-weight:bold;");
    m_discountLabel->hide();
    refreshCart();
    loadProducts();
}