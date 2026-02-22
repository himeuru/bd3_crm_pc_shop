#pragma once
#include <QWidget>
#include <QTableView>
#include <QSqlQueryModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QSortFilterProxyModel>

class SqlTableWidget : public QWidget {
    Q_OBJECT
public:
    explicit SqlTableWidget(QWidget* parent = nullptr);

    void setQuery(const QString& sql,
                  const QStringList& headers = {});
    void refresh();
    void setTitle(const QString& title);
    void setSearchable(bool on);

    QModelIndex currentIndex() const;
    QVariant    currentData(int column) const;
    int         currentRow() const;

signals:
    void rowSelected(int row);
    void rowDoubleClicked(int row);

private slots:
    void onSearchChanged(const QString& text);

private:
    void applyHeaders(const QStringList& headers);

    QLabel*               m_title    = nullptr;
    QLineEdit*            m_search   = nullptr;
    QTableView*           m_view     = nullptr;
    QSqlQueryModel*       m_model    = nullptr;
    QSortFilterProxyModel* m_proxy   = nullptr;
    QString               m_sql;
    QStringList           m_headers;
};
