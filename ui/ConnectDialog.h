#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>

class ConnectDialog : public QDialog {
    Q_OBJECT
public:
    explicit ConnectDialog(QWidget* parent = nullptr);

private slots:
    void onTestClicked();
    void onConnectClicked();

private:
    void buildUi();
    void loadFromConfig();
    void saveToConfig();
    bool tryConnect();
    void setStatus(const QString& msg, bool ok);

    QLineEdit*   m_host;
    QSpinBox*    m_port;
    QLineEdit*   m_dbName;
    QLineEdit*   m_user;
    QLineEdit*   m_password;
    QLabel*      m_status;
    QPushButton* m_btnTest;
    QPushButton* m_btnConnect;
};
