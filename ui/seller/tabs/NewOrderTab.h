#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
#include "../../widgets/SqlTableWidget.h"

struct CartItem {
    int     productId;
    QString name;
    QString category;
    double  costPrice;
    double  salePrice;
    int     qty;
};

class NewOrderTab : public QWidget {
    Q_OBJECT
public:
    explicit NewOrderTab(QWidget* parent = nullptr);

public slots:
    void reloadProducts();  // вызывается при переключении на вкладку

private slots:
    void onSearch();
    void onAddToCart();
    void onRemoveFromCart();
    void onClearCart();
    void onPlaceOrder();
    void onClientSearch();
    void onClearClient();

private:
    void buildUi();
    void loadProducts();
    void refreshCart();
    void updateOrderButton();

    // Каталог
    SqlTableWidget* m_productsTable  = nullptr;
    QLineEdit*      m_searchEdit     = nullptr;
    QComboBox*      m_categoryFilter = nullptr;
    QSpinBox*       m_qtySpinner     = nullptr;
    QPushButton*    m_btnAddCart     = nullptr;
    QLabel*         m_stockHint      = nullptr;

    // Клиент
    QLineEdit*      m_clientSearch    = nullptr;
    QPushButton*    m_btnClientSearch = nullptr;
    QLabel*         m_clientLabel     = nullptr;
    QLabel*         m_discountLabel   = nullptr;

    // Корзина
    QTableWidget*   m_cartTable           = nullptr;
    QLabel*         m_cartCountLabel      = nullptr;
    QLabel*         m_totalLabel          = nullptr;
    QLabel*         m_discountSavingLabel = nullptr;
    QPushButton*    m_btnPlaceOrder       = nullptr;

    QList<CartItem> m_cart;
    int     m_currentClientId = -1;
    double  m_clientDiscount  = 0.0;
    QString m_clientName;
};