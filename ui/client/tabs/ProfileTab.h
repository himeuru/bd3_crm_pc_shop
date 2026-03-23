#pragma once
#include <QWidget>

class ProfileTab : public QWidget {
    Q_OBJECT
public:
    explicit ProfileTab(QWidget* parent = nullptr);
private slots:
    void onChangePassword();
private:
    void buildUi();
};