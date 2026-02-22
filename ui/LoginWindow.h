#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class LoginWindow : public QWidget {
    Q_OBJECT
public:
    explicit LoginWindow(QWidget* parent = nullptr);

private slots:
    void onLoginClicked();
    void onSettingsClicked();
    void onPasswordToggle();

private:
    void buildUi();
    void openRoleWindow(const QString& role);

    QLineEdit*   m_login;
    QLineEdit*   m_password;
    QPushButton* m_btnLogin;
    QPushButton* m_btnTogglePass;
    QLabel*      m_errorLabel;
    QLabel*      m_connLabel;
};
