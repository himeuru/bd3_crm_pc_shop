#include "NewOrderTab.h"
#include "../../../core/DatabaseManager.h"
#include "../../../core/Session.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

NewOrderTab::NewOrderTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
}

void NewOrderTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto* clientRow = new QHBoxLayout();
    m_clientSearch = new QLineEdit(this);
    m_clientSearch->setPlaceholderText("Логин или телефон клиента...");
    auto* btnClientSearch = new QPushButton("Найти клиента", this);
    m_clientLabel = new QLabel("Клиент не выбран", this);
    m_clientLabel->setStyleSheet("color: #f38ba8; font-weight: bold;");
    m_discountLabel = new QLabel("", this);
    m_discountLabel->setStyleSheet("color: #a6e3a1;");
    clientRow->addWidget(m_clientSearch);
    clientRow->addWidget(btnClientSearch);
    clientRow->addWidget(m_clientLabel);
    clientRow->addWidget(m_discountLabel);
    clientRow->addStretch();

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto* searchRow = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("Поиск товара...");
    auto* btnSearch = new QPushButton("Найти", this);
    m_qtySpinner = new QSpinBox(this);
    m_qtySpinner->setRange(1, 999);
    m_qtySpinner->setValue(1);
    m_qtySpinner->setPrefix("Кол-во: ");
    m_btnAddCart = new QPushButton("+ В корзину", this);
    m_btnAddCart->setObjectName("btnSuccess");
    m_btnAddCart->setEnabled(false);
    searchRow->addWidget(m_searchEdit);
    searchRow->addWidget(btnSearch);
    searchRow->addWidget(m_qtySpinner);
    searchRow->addWidget(m_btnAddCart);

    m_productsTable = new SqlTableWidget(leftPanel);
    m_productsTable->setTitle("Товары на складе");

    leftLayout->addLayout(searchRow);
    leftLayout->addWidget(m_productsTable);

    auto* rightPanel  = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* cartLabel = new QLabel("🛒 Корзина", rightPanel);
    cartLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #89b4fa;");

    m_cartTable = new QTableWidget(0, 5, rightPanel);
    m_cartTable->setHorizontalHeaderLabels({"Товар", "Кат.", "Цена", "Кол-во", "Итого"});
    m_cartTable->horizontalHeader()->setStretchLastSection(true);
    m_cartTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_cartTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_cartTable->verticalHeader()->hide();
    m_cartTable->setAlternatingRowColors(true);

    auto* cartBtnRow = new QHBoxLayout();
    auto* btnRemove  = new QPushButton("Удалить из корзины", rightPanel);
    btnRemove->setObjectName("btnDanger");
    m_totalLabel = new QLabel("Итого: 0 ₽", rightPanel);
    m_totalLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #89b4fa;");

    cartBtnRow->addWidget(btnRemove);
    cartBtnRow->addStretch();
    cartBtnRow->addWidget(m_totalLabel);

    m_btnPlaceOrder = new QPushButton("✓ Оформить заказ", rightPanel);
    m_btnPlaceOrder->setMinimumHeight(40);
    m_btnPlaceOrder->setEnabled(false);

    rightLayout->addWidget(cartLabel);
    rightLayout->addWidget(m_cartTable);
    rightLayout->addLayout(cartBtnRow);
    rightLayout->addWidget(m_btnPlaceOrder);

    splitter->addWidget(leftPanel);
    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    layout->addLayout(clientRow);
    layout->addWidget(splitter);

    connect(btnSearch,        &QPushButton::clicked, this, &NewOrderTab::onSearch);
    connect(m_searchEdit,     &QLineEdit::returnPressed, this, &NewOrderTab::onSearch);
    connect(m_btnAddCart,     &QPushButton::clicked, this, &NewOrderTab::onAddToCart);
    connect(btnRemove,        &QPushButton::clicked, this, &NewOrderTab::onRemoveFromCart);
    connect(m_btnPlaceOrder,  &QPushButton::clicked, this, &NewOrderTab::onPlaceOrder);
    connect(btnClientSearch,  &QPushButton::clicked, this, &NewOrderTab::onClientSearch);

    connect(m_productsTable, &SqlTableWidget::rowSelected,
            this, [this](int row) { m_btnAddCart->setEnabled(row >= 0); });
}

void NewOrderTab::onSearch()
{
    QString text = m_searchEdit->text().trimmed();
    int shopId = Session::instance().shopId;

    QString sql;
    if (text.isEmpty()) {
        sql = QString(
            "SELECT "
            "  p.id, p.name, p.category, ws.quantity, "
            "  calc_sale_price(p.cost_price, e.salary_pct, s.director_margin, %2) AS \"Цена\" "
            "FROM warehouse_stock ws "
            "JOIN warehouses  w  ON w.id  = ws.warehouse_id AND w.shop_id = %1 AND w.type = 'micro' "
            "JOIN products    p  ON p.id  = ws.product_id AND p.is_active "
            "JOIN shops       s  ON s.id  = %1 "
            "JOIN employees   e  ON e.id  = %3 "
            "WHERE ws.quantity > 0 "
            "ORDER BY p.category, p.name"
        ).arg(shopId).arg(m_clientDiscount).arg(Session::instance().userId);
    } else {
        sql = QString(
            "SELECT "
            "  p.id, p.name, p.category, ws.quantity, "
            "  calc_sale_price(p.cost_price, e.salary_pct, s.director_margin, %3) AS \"Цена\" "
            "FROM warehouse_stock ws "
            "JOIN warehouses  w  ON w.id  = ws.warehouse_id AND w.shop_id = %1 AND w.type = 'micro' "
            "JOIN products    p  ON p.id  = ws.product_id AND p.is_active "
            "JOIN shops       s  ON s.id  = %1 "
            "JOIN employees   e  ON e.id  = %4 "
            "WHERE ws.quantity > 0 "
            "  AND (lower(p.name) %% lower('%2') OR p.search_vec @@ plainto_tsquery('russian', '%2')) "
            "ORDER BY p.category, p.name"
        ).arg(shopId).arg(text).arg(m_clientDiscount).arg(Session::instance().userId);
    }

    m_productsTable->setQuery(sql, {"ID", "Название", "Категория", "На складе", "Цена"});
}

void NewOrderTab::onAddToCart()
{
    int     productId = m_productsTable->currentData(0).toInt();
    QString name      = m_productsTable->currentData(1).toString();
    QString category  = m_productsTable->currentData(2).toString();
    int     inStock   = m_productsTable->currentData(3).toInt();
    double  price     = m_productsTable->currentData(4).toDouble();
    int     qty       = m_qtySpinner->value();

    if (productId <= 0) return;
    if (qty > inStock) {
        QMessageBox::warning(this, "Нет на складе",
            QString("На складе только %1 шт.").arg(inStock));
        return;
    }

    for (auto& item : m_cart) {
        if (item.productId == productId) {
            item.qty += qty;
            refreshCart();
            return;
        }
    }

    m_cart.append({productId, name, category, 0, price, qty});
    refreshCart();
}

void NewOrderTab::onRemoveFromCart()
{
    int row = m_cartTable->currentRow();
    if (row < 0 || row >= m_cart.size()) return;
    m_cart.removeAt(row);
    refreshCart();
}

void NewOrderTab::refreshCart()
{
    m_cartTable->setRowCount(m_cart.size());
    for (int i = 0; i < m_cart.size(); ++i) {
        const auto& item = m_cart[i];
        m_cartTable->setItem(i, 0, new QTableWidgetItem(item.name));
        m_cartTable->setItem(i, 1, new QTableWidgetItem(item.category));
        m_cartTable->setItem(i, 2, new QTableWidgetItem(QString::number(item.salePrice, 'f', 2)));
        m_cartTable->setItem(i, 3, new QTableWidgetItem(QString::number(item.qty)));
        m_cartTable->setItem(i, 4, new QTableWidgetItem(
            QString::number(item.salePrice * item.qty, 'f', 2)));
    }
    refreshTotal();
    m_btnPlaceOrder->setEnabled(!m_cart.isEmpty() && m_currentClientId > 0);
}

void NewOrderTab::refreshTotal()
{
    double total = 0;
    for (const auto& item : m_cart) total += item.salePrice * item.qty;
    m_totalLabel->setText(QString("Итого: %1 ₽").arg(total, 0, 'f', 2));
}

void NewOrderTab::onClientSearch()
{
    QString q = m_clientSearch->text().trimmed();
    if (q.isEmpty()) return;

    QSqlQuery sq(DatabaseManager::instance().db());
    sq.prepare(
        "SELECT id, full_name, discount_pct FROM clients "
        "WHERE login = :q OR phone = :q LIMIT 1");
    sq.bindValue(":q", q);

    if (!sq.exec() || !sq.next()) {
        m_clientLabel->setText("Клиент не найден");
        m_clientLabel->setStyleSheet("color: #f38ba8; font-weight: bold;");
        m_currentClientId = -1;
        m_clientDiscount  = 0;
        return;
    }

    m_currentClientId = sq.value("id").toInt();
    m_clientName      = sq.value("full_name").toString();
    m_clientDiscount  = sq.value("discount_pct").toDouble();

    m_clientLabel->setText(m_clientName);
    m_clientLabel->setStyleSheet("color: #a6e3a1; font-weight: bold;");
    m_discountLabel->setText(QString("Скидка: %1%").arg(m_clientDiscount * 100, 0, 'f', 1));

    m_btnPlaceOrder->setEnabled(!m_cart.isEmpty() && m_currentClientId > 0);
}

void NewOrderTab::onPlaceOrder()
{
    if (m_currentClientId <= 0) {
        QMessageBox::warning(this, "Клиент", "Выберите клиента перед оформлением");
        return;
    }
    if (m_cart.isEmpty()) {
        QMessageBox::warning(this, "Корзина", "Корзина пуста");
        return;
    }

    QJsonArray arr;
    for (const auto& item : m_cart) {
        QJsonObject obj;
        obj["product_id"] = item.productId;
        obj["qty"]        = item.qty;
        arr.append(obj);
    }

    QSqlQuery q(DatabaseManager::instance().db());
    q.prepare("SELECT create_order(:shop, :client, :seller, :items::jsonb)");
    q.bindValue(":shop",   Session::instance().shopId);
    q.bindValue(":client", m_currentClientId);
    q.bindValue(":seller", Session::instance().userId);
    q.bindValue(":items",  QJsonDocument(arr).toJson(QJsonDocument::Compact));

    if (!q.exec() || !q.next()) {
        QString err = q.lastError().text();
        if (err.contains("INSUFFICIENT_STOCK"))
            QMessageBox::warning(this, "Нет товара", "Недостаточно товара на складе");
        else if (err.contains("SHOP_CLOSED"))
            QMessageBox::warning(this, "Магазин закрыт", "Магазин закрыт, продажи недоступны");
        else
            QMessageBox::critical(this, "Ошибка", err);
        return;
    }

    int orderId = q.value(0).toInt();
    QMessageBox::information(this, "Заказ оформлен",
        QString("Заказ #%1 успешно оформлен!\nКлиент: %2")
            .arg(orderId).arg(m_clientName));

    m_cart.clear();
    m_currentClientId = -1;
    m_clientLabel->setText("Клиент не выбран");
    m_clientLabel->setStyleSheet("color: #f38ba8; font-weight: bold;");
    m_discountLabel->clear();
    m_clientSearch->clear();
    refreshCart();
}
