#include "CartTab.h"
#include <QVBoxLayout>

CartTab::CartTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
}

void CartTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(40, 40, 40, 40);

    m_infoLabel = new QLabel(
        "Для оформления заказа обратитесь к продавцу.\n\n"
        "Продавец найдёт вас по логину или номеру телефона\n"
        "и оформит заказ с учётом вашей персональной скидки.", this);
    m_infoLabel->setAlignment(Qt::AlignCenter);
    m_infoLabel->setWordWrap(true);
    m_infoLabel->setStyleSheet("color: #6c7086; font-size: 14px; line-height: 1.6;");

    layout->addStretch();
    layout->addWidget(m_infoLabel);
    layout->addStretch();
}
