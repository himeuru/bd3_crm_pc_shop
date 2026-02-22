#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"

class StockTab : public QWidget {
    Q_OBJECT
public:
    explicit StockTab(QWidget* parent = nullptr);
    void reload();
private:
    void buildUi();
    SqlTableWidget* m_table;
};
