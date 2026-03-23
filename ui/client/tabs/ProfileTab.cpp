#include "ProfileTab.h"
#include "../../../core/Session.h"
#include "../../../repositories/AuthRepository.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFrame>
#include <QSqlQuery>
#include <QDateTime>
#include "../../../core/DatabaseManager.h"

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

    // ── Заголовок ─────────────────────────────────────────────────────
    auto* titleLabel = new QLabel("Личный кабинет", this);
    titleLabel->setStyleSheet(
        "font-size: 20px; font-weight: bold; color: #89b4fa;");

    // ── Карточка с информацией ────────────────────────────────────────
    auto* infoFrame = new QFrame(this);
    infoFrame->setFrameShape(QFrame::StyledPanel);
    infoFrame->setStyleSheet(
        "background: #313244; border-radius: 8px; border: 1px solid #45475a;");
    auto* infoLayout = new QVBoxLayout(infoFrame);
    infoLayout->setContentsMargins(20, 14, 20, 14);
    infoLayout->setSpacing(6);

    auto* nameLabel = new QLabel(
        QString("Имя: <b>%1</b>").arg(s.fullName), this);
    nameLabel->setStyleSheet("font-size: 14px; color: #cdd6f4;");

    // Загрузить телефон и скидку из БД
    QSqlQuery q(DatabaseManager::instance().db());
    QString infoSql = QString(
                          "SELECT phone, email, discount_pct, registered_at "
                          "FROM clients WHERE id = %1"
                          ).arg(s.userId);

    QString phoneText = "—", emailText = "—", discountText = "0%", regText = "";
    if (q.exec(infoSql) && q.next()) {
        phoneText   = q.value("phone").toString().isEmpty()
        ? "—" : q.value("phone").toString();
        emailText   = q.value("email").toString().isEmpty()
                        ? "—" : q.value("email").toString();
        double disc = q.value("discount_pct").toDouble();
        discountText = disc > 0.0001
                           ? QString("%1%").arg(disc * 100, 0, 'f', 1) : "нет";
        regText = q.value("registered_at").toDateTime()
                      .toString("dd.MM.yyyy");
    }

    auto* phoneLabel   = new QLabel(QString("Телефон: <b>%1</b>").arg(phoneText), this);
    phoneLabel->setStyleSheet("font-size: 13px; color: #cdd6f4;");
    auto* emailLabel   = new QLabel(QString("Email: <b>%1</b>").arg(emailText), this);
    emailLabel->setStyleSheet("font-size: 13px; color: #cdd6f4;");
    auto* discLabel    = new QLabel(
        QString("Персональная скидка: <b style='color:#a6e3a1'>%1</b>").arg(discountText), this);
    discLabel->setStyleSheet("font-size: 13px; color: #cdd6f4;");
    auto* regLabel     = new QLabel(
        QString("Зарегистрирован: %1").arg(regText), this);
    regLabel->setStyleSheet("font-size: 12px; color: #6c7086;");

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(phoneLabel);
    infoLayout->addWidget(emailLabel);
    infoLayout->addWidget(discLabel);
    infoLayout->addWidget(regLabel);

    // ── Смена пароля ──────────────────────────────────────────────────
    auto* group = new QGroupBox("Смена пароля", this);
    group->setStyleSheet("QGroupBox { font-size: 14px; font-weight: bold; }");
    auto* form  = new QFormLayout(group);
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight);

    auto* editOld = new QLineEdit(group);
    editOld->setEchoMode(QLineEdit::Password);
    editOld->setPlaceholderText("Введите текущий пароль");

    auto* editNew = new QLineEdit(group);
    editNew->setEchoMode(QLineEdit::Password);
    editNew->setPlaceholderText("Минимум 6 символов");

    auto* editConfirm = new QLineEdit(group);
    editConfirm->setEchoMode(QLineEdit::Password);
    editConfirm->setPlaceholderText("Повторите новый пароль");

    auto* statusLabel = new QLabel("", group);
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("color: #f38ba8; font-size: 12px;");

    auto* btnChange = new QPushButton("Сменить пароль", group);
    btnChange->setMinimumHeight(36);
    btnChange->setMinimumWidth(160);

    form->addRow("Текущий пароль:", editOld);
    form->addRow("Новый пароль:",   editNew);
    form->addRow("Подтверждение:",  editConfirm);
    form->addRow("",                statusLabel);
    form->addRow("",                btnChange);

    layout->addWidget(titleLabel);
    layout->addWidget(infoFrame);
    layout->addSpacing(8);
    layout->addWidget(group);
    layout->addStretch();

    // Один connect — без дублирования
    connect(btnChange, &QPushButton::clicked, [this, editOld, editNew, editConfirm, statusLabel]() {
        statusLabel->clear();

        QString oldPass = editOld->text();
        QString newPass = editNew->text();
        QString confirm = editConfirm->text();

        if (oldPass.isEmpty() || newPass.isEmpty() || confirm.isEmpty()) {
            statusLabel->setText("Заполните все поля");
            return;
        }
        if (newPass.length() < 6) {
            statusLabel->setText("Новый пароль должен быть не менее 6 символов");
            return;
        }
        if (newPass != confirm) {
            statusLabel->setText("Новые пароли не совпадают");
            editConfirm->clear();
            editConfirm->setFocus();
            return;
        }

        AuthRepository repo;
        bool ok = repo.changePassword(
            Session::instance().userId, "client", oldPass, newPass);

        if (!ok) {
            statusLabel->setText("Неверный текущий пароль");
            statusLabel->setStyleSheet("color: #f38ba8; font-size: 12px;");
            editOld->clear();
            editOld->setFocus();
        } else {
            statusLabel->setText("Пароль успешно изменён");
            statusLabel->setStyleSheet("color: #a6e3a1; font-size: 12px;");
            editOld->clear();
            editNew->clear();
            editConfirm->clear();
        }
    });
}

void ProfileTab::onChangePassword() {}