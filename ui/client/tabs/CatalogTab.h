#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"

class CatalogTab : public QWidget {
    Q_OBJECT
public:
    explicit CatalogTab(QWidget* parent = nullptr);
private:
    void buildUi();
    SqlTableWidget* m_table;
};
