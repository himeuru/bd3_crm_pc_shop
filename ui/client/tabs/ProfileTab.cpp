#include "ProfileTab.h"
#include "../../../core/Session.h"
#include "../../../repositories/AuthRepository.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

ProfileTab::ProfileTab(QWidget* parent) : QWidget(parent)
{
    buildUi();
}

void ProfileTab::buildUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(60, 40, 60, 40);
    layout->setSpacing(16);

    auto& s = Session::instance();

    auto* titleLabel = new QLabel("Личный кабинет", this);
    titleLabel->setObjectName("labelTitle");

    auto* nameLabel = new QLabel(
        QString("Имя: <b>%1</b>").arg(s.fullName), this);
    nameLabel->setStyleSheet("font-size: 14px;");

    auto* group = new QGroupBox("Смена пароля", this);
    auto* form  = new QFormLayout(group);

    auto* editOld     = new QLineEdit(group);
    editOld->setEchoMode(QLineEdit::Password);
    editOld->setPlaceholderText("Текущий пароль");

    auto* editNew     = new QLineEdit(group);
    editNew->setEchoMode(QLineEdit::Password);
    editNew->setPlaceholderText("Новый пароль (мин. 6 символов)");

    auto* editConfirm = new QLineEdit(group);
    editConfirm->setEchoMode(QLineEdit::Password);
    editConfirm->setPlaceholderText("Повторите новый пароль");

    auto* btnChange = new QPushButton("Сменить пароль", group);
    btnChange->setMinimumHeight(36);

    form->addRow("Текущий пароль:", editOld);
    form->addRow("Новый пароль:",   editNew);
    form->addRow("Подтверждение:",  editConfirm);
    form->addRow("",                btnChange);

    layout->addWidget(titleLabel);
    layout->addWidget(nameLabel);
    layout->addSpacing(16);
    layout->addWidget(group);
    layout->addStretch();

    connect(btnChange, &QPushButton::clicked, this, [this, editOld, editNew, editConfirm]() {
        onChangePassword();
        Q_UNUSED(editOld); Q_UNUSED(editNew); Q_UNUSED(editConfirm);
    });

    connect(btnChange, &QPushButton::clicked, [=]() {
        if (editNew->text().length() < 6) {
            QMessageBox::warning(this, "Слишком короткий", "Пароль должен быть не менее 6 символов");
            return;
        }
        if (editNew->text() != editConfirm->text()) {
            QMessageBox::warning(this, "Ошибка", "Новые пароли не совпадают");
            return;
        }
        AuthRepository repo;
        bool ok = repo.changePassword(
            Session::instance().userId, "client",
            editOld->text(), editNew->text());
        if (!ok)
            QMessageBox::warning(this, "Ошибка", "Неверный текущий пароль");
        else {
            QMessageBox::information(this, "Готово", "Пароль успешно изменён");
            editOld->clear();
            editNew->clear();
            editConfirm->clear();
        }
    });
}

void ProfileTab::onChangePassword() {}
