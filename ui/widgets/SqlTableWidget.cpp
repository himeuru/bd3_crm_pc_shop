#include "SqlTableWidget.h"
#include <QSqlRecord>
#include "../../core/DatabaseManager.h"

SqlTableWidget::SqlTableWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    auto* topBar = new QHBoxLayout();

    m_title = new QLabel(this);
    m_title->setStyleSheet("font-size: 15px; font-weight: bold; color: #89b4fa;");

    m_search = new QLineEdit(this);
    m_search->setPlaceholderText("Поиск...");
    m_search->setMaximumWidth(280);
    m_search->hide();

    topBar->addWidget(m_title);
    topBar->addStretch();
    topBar->addWidget(m_search);

    m_model = new QSqlQueryModel(this);
    m_proxy = new QSortFilterProxyModel(this);
    m_proxy->setSourceModel(m_model);
    m_proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_proxy->setFilterKeyColumn(-1);

    m_view = new QTableView(this);
    m_view->setModel(m_proxy);
    m_view->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_view->setSelectionMode(QAbstractItemView::SingleSelection);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_view->setAlternatingRowColors(true);
    m_view->verticalHeader()->hide();
    m_view->horizontalHeader()->setStretchLastSection(true);
    m_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    m_view->setSortingEnabled(true);
    m_view->setShowGrid(true);

    mainLayout->addLayout(topBar);
    mainLayout->addWidget(m_view);

    connect(m_search, &QLineEdit::textChanged,
            this,     &SqlTableWidget::onSearchChanged);

    connect(m_view->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, [this](const QModelIndex& cur, const QModelIndex&) {
        if (cur.isValid()) emit rowSelected(m_proxy->mapToSource(cur).row());
    });

    connect(m_view, &QTableView::doubleClicked,
            this, [this](const QModelIndex& idx) {
        emit rowDoubleClicked(m_proxy->mapToSource(idx).row());
    });
}

void SqlTableWidget::setQuery(const QString& sql, const QStringList& headers)
{
    m_sql     = sql;
    m_headers = headers;
    refresh();
}

void SqlTableWidget::refresh()
{
    if (m_sql.isEmpty()) return;
    m_model->setQuery(m_sql, DatabaseManager::instance().db());
    if (m_model->lastError().isValid()) {
        qWarning() << "[SqlTableWidget] query error:" << m_model->lastError().text();
    }
    applyHeaders(m_headers);
    m_view->resizeColumnsToContents();
}

void SqlTableWidget::setTitle(const QString& title)
{
    m_title->setText(title);
}

void SqlTableWidget::setSearchable(bool on)
{
    m_search->setVisible(on);
}

QModelIndex SqlTableWidget::currentIndex() const
{
    return m_proxy->mapToSource(m_view->currentIndex());
}

QVariant SqlTableWidget::currentData(int column) const
{
    int row = m_proxy->mapToSource(m_view->currentIndex()).row();
    if (row < 0) return {};
    return m_model->record(row).value(column);
}

int SqlTableWidget::currentRow() const
{
    return m_proxy->mapToSource(m_view->currentIndex()).row();
}

void SqlTableWidget::onSearchChanged(const QString& text)
{
    m_proxy->setFilterFixedString(text);
}

void SqlTableWidget::applyHeaders(const QStringList& headers)
{
    for (int i = 0; i < headers.size() && i < m_model->columnCount(); ++i)
        m_model->setHeaderData(i, Qt::Horizontal, headers[i]);
}
