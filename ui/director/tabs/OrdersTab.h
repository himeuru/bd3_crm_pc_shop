#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"
#include <QPushButton>
#include <QComboBox>
#include <QDateEdit>
#include <QLabel>

class OrdersTab : public QWidget {
    Q_OBJECT
public:
    explicit OrdersTab(QWidget* parent = nullptr);
    void reload();
private slots:
    void onCancel();
    void onFilter();
    void onSelectionChanged(int row);
private:
    void buildUi();
    SqlTableWidget* m_table;
    SqlTableWidget* m_itemsTable;
    QPushButton*    m_btnCancel;
    QComboBox*      m_filterStatus;
    QDateEdit*      m_dateFrom;
    QDateEdit*      m_dateTo;
};
