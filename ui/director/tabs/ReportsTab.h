#pragma once
#include <QWidget>
#include "../../widgets/SqlTableWidget.h"
#include <QComboBox>
#include <QDateEdit>
#include <QPushButton>

class ReportsTab : public QWidget {
    Q_OBJECT
public:
    explicit ReportsTab(QWidget* parent = nullptr);
    void reload();
private slots:
    void onShowReport();
private:
    void buildUi();
    SqlTableWidget* m_table;
    QComboBox*      m_reportType;
    QDateEdit*      m_dateFrom;
    QDateEdit*      m_dateTo;
};
