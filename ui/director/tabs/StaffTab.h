#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>

class StaffTab : public QWidget {
    Q_OBJECT
public:
    explicit StaffTab(QWidget* parent = nullptr);
    void reload();
private slots:
    void onHire();
    void onFire();
    void onChangeSalary();
    void onSelectionChanged(int row);
private:
    void buildUi();
    SqlTableWidget* m_table;
    QPushButton*    m_btnHire;
    QPushButton*    m_btnFire;
    QPushButton*    m_btnSalary;
};
