#pragma once
#include <QMainWindow>
#include <QTabWidget>
#include <QTimer>

class ClientMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit ClientMainWindow(QWidget* parent = nullptr);
private slots:
    void onLogout();
private:
    void buildUi();
    QTabWidget* m_tabs;
};
