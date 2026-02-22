#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
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

private slots:
    void onSearch();
    void onAddToCart();
    void onRemoveFromCart();
    void onPlaceOrder();
    void onClientSearch();

private:
    void buildUi();
    void refreshCart();
    void refreshTotal();
    double computeTotal() const;

    SqlTableWidget* m_productsTable;
    QTableWidget*   m_cartTable;
    QLineEdit*      m_searchEdit;
    QLineEdit*      m_clientSearch;
    QLabel*         m_clientLabel;
    QLabel*         m_totalLabel;
    QLabel*         m_discountLabel;
    QSpinBox*       m_qtySpinner;
    QPushButton*    m_btnAddCart;
    QPushButton*    m_btnPlaceOrder;

    QList<CartItem> m_cart;
    int     m_currentClientId = -1;
    double  m_clientDiscount  = 0.0;
    QString m_clientName;
};
