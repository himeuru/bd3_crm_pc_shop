#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"

class OrderHistoryTab : public QWidget {
    Q_OBJECT
public:
    explicit OrderHistoryTab(QWidget* parent = nullptr);
    void reload();
private:
    void buildUi();
    SqlTableWidget* m_table;
    SqlTableWidget* m_itemsTable;
};
