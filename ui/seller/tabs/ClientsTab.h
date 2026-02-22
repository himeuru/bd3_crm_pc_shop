#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"

class ClientsTab : public QWidget {
    Q_OBJECT
public:
    explicit ClientsTab(QWidget* parent = nullptr);
    void reload();
private slots:
    void onRegisterClient();
    void onSetDiscount();
private:
    void buildUi();
    SqlTableWidget* m_table;
};
