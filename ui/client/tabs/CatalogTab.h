#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include "../../widgets/SqlTableWidget.h"

class CatalogTab : public QWidget {
    Q_OBJECT
public:
    explicit CatalogTab(QWidget* parent = nullptr);

private slots:
    void onSearch();

private:
    void buildUi();

    SqlTableWidget* m_table          = nullptr;
    QLineEdit*      m_searchEdit     = nullptr;
    QComboBox*      m_categoryFilter = nullptr;
    double          m_clientDiscount = 0.0;
};