#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"
#include <QPushButton>

class ShopsTab : public QWidget {
    Q_OBJECT
public:
    explicit ShopsTab(QWidget* parent = nullptr);
    void reload();

private slots:
    void onOpenShop();
    void onCloseShop();
    void onSelectionChanged(int row);

private:
    void buildUi();
    int  selectedShopId() const;

    SqlTableWidget* m_table;
    QPushButton*    m_btnOpen;
    QPushButton*    m_btnClose;
};
