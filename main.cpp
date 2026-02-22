#include <QApplication>
#include "core/AppConfig.h"
#include "ui/ConnectDialog.h"
#include "ui/LoginWindow.h"
#include "ui/SetupDialog.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("CRM PC Shop");
    app.setOrganizationName("PCShopCorp");

    app.setStyleSheet(
        "QMainWindow, QDialog { background: #1e1e2e; }"
        "QWidget { background: #1e1e2e; color: #cdd6f4; font-family: 'Segoe UI'; font-size: 13px; }"
        "QTabWidget::pane { border: 1px solid #313244; border-radius: 4px; }"
        "QTabBar::tab { background: #313244; color: #bac2de; padding: 8px 18px; border-radius: 4px 4px 0 0; margin-right: 2px; }"
        "QTabBar::tab:selected { background: #89b4fa; color: #1e1e2e; font-weight: bold; }"
        "QPushButton { background: #89b4fa; color: #1e1e2e; border: none; border-radius: 6px; padding: 7px 18px; font-weight: bold; }"
        "QPushButton:hover { background: #b4befe; }"
        "QPushButton:pressed { background: #74c7ec; }"
        "QPushButton[flat=true] { background: transparent; color: #89b4fa; }"
        "QPushButton#btnDanger { background: #f38ba8; }"
        "QPushButton#btnDanger:hover { background: #eba0ac; }"
        "QPushButton#btnSuccess { background: #a6e3a1; color: #1e1e2e; }"
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: #313244; border: 1px solid #45475a; border-radius: 6px; padding: 6px 10px; color: #cdd6f4; }"
        "QLineEdit:focus, QSpinBox:focus, QComboBox:focus { border-color: #89b4fa; }"
        "QTableView { background: #181825; border: 1px solid #313244; border-radius: 4px; gridline-color: #313244; alternate-background-color: #1e1e2e; selection-background-color: #313244; }"
        "QTableView::item:selected { background: #45475a; color: #cdd6f4; }"
        "QHeaderView::section { background: #313244; color: #89b4fa; padding: 6px; border: none; font-weight: bold; }"
        "QScrollBar:vertical { background: #181825; width: 8px; border-radius: 4px; }"
        "QScrollBar::handle:vertical { background: #45475a; border-radius: 4px; }"
        "QLabel { color: #cdd6f4; }"
        "QLabel#labelTitle { font-size: 22px; font-weight: bold; color: #89b4fa; }"
        "QLabel#labelSubtitle { color: #6c7086; font-size: 12px; }"
        "QGroupBox { border: 1px solid #313244; border-radius: 6px; margin-top: 12px; padding-top: 8px; color: #bac2de; font-weight: bold; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; color: #89b4fa; }"
        "QMessageBox { background: #1e1e2e; }"
        "QStatusBar { background: #181825; color: #6c7086; }"
        );

    AppConfig::instance().load();

    // ── Шаг 1: подключение к БД ─────────────────────────────────────────
    ConnectDialog connectDlg;
    if (connectDlg.exec() != QDialog::Accepted)
        return 0;

    // ── Шаг 2: первый запуск? ────────────────────────────────────────────
    // Проверяем есть ли хоть один сотрудник в БД.
    // Если нет — показываем мастер создания первого магазина и директора.
    if (SetupDialog::isFirstRun()) {
        SetupDialog setupDlg;
        if (setupDlg.exec() != QDialog::Accepted)
            return 0;  // пользователь закрыл мастер — выходим
        // После SetupDialog директор уже создан — показываем окно входа
        // с подсказкой что нужно войти с только что созданными данными
    }

    // ── Шаг 3: вход в систему ────────────────────────────────────────────
    LoginWindow loginWin;
    loginWin.show();

    return app.exec();
}
