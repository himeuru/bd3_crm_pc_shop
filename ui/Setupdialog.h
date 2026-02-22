#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTimer>

class SetupDialog : public QDialog {
    Q_OBJECT
public:
    explicit SetupDialog(QWidget* parent = nullptr);

    static bool isFirstRun();

private slots:
    void onCreateClicked();

private:
    void buildUi();
    bool validate();
    void setStatus(const QString& msg, bool ok);

    // Магазин
    QLineEdit* m_shopName;
    QLineEdit* m_shopAddress;

    // Директор
    QLineEdit* m_fullName;
    QLineEdit* m_login;
    QLineEdit* m_password;
    QLineEdit* m_passwordConfirm;
    QSpinBox*  m_salary;

    QPushButton* m_btnCreate;
    QLabel*      m_status;

    int m_createdShopId = -1;  // сохраняем id созданного магазина между запросами
};
