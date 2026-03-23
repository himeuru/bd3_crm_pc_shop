#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include "../../widgets/SqlTableWidget.h"

struct WishItem {
    int     productId;
    QString name;
    QString category;
    double  price;
    int     qty;
};

class CartTab : public QWidget {
    Q_OBJECT
public:
    explicit CartTab(QWidget* parent = nullptr);

private slots:
    void onSearch();
    void onAddToWish();
    void onRemoveFromWish();
    void onClearWish();
    void onCopyList();

private:
    void buildUi();
    void loadProducts();
    void refreshWish();

    // Каталог
    SqlTableWidget* m_catalogTable   = nullptr;
    QLineEdit*      m_searchEdit     = nullptr;
    QComboBox*      m_categoryFilter = nullptr;
    QPushButton*    m_btnAdd         = nullptr;
    QLabel*         m_stockHint      = nullptr;

    // Список желаний
    QTableWidget*   m_wishTable      = nullptr;
    QLabel*         m_totalLabel     = nullptr;
    QLabel*         m_discountInfo   = nullptr;

    QList<WishItem> m_wish;
    double          m_clientDiscount = 0.0;
};