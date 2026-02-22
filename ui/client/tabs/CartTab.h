#pragma once
#include <QWidget>
#include <QLabel>

class CartTab : public QWidget {
    Q_OBJECT
public:
    explicit CartTab(QWidget* parent = nullptr);
private:
    void buildUi();
    QLabel* m_infoLabel;
};
