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

    // ── Тема: тёмная с высоким контрастом ────────────────────────────────
    // Основные цвета:
    //   фон приложения : #0f0f17  (очень тёмный, почти чёрный)
    //   фон панелей    : #1a1b26  (тёмно-синий)
    //   фон элементов  : #24253a  (чуть светлее)
    //   граница        : #3b3d57
    //   текст основной : #e2e4f0  (почти белый, высокий контраст)
    //   текст вторичный: #9899b3
    //   акцент синий   : #7aa2f7
    //   акцент зелёный : #73daca
    //   акцент красный : #f7768e
    //   акцент жёлтый  : #e0af68
    app.setStyleSheet(
        // Базовые контейнеры
        "QMainWindow, QDialog { background: #0f0f17; }"
        "QWidget { background: #0f0f17; color: #e2e4f0;"
        "  font-family: 'Segoe UI'; font-size: 13px; }"

        // Вкладки
        "QTabWidget::pane { border: 1px solid #3b3d57; background: #0f0f17; }"
        "QTabBar::tab { background: #1a1b26; color: #9899b3;"
        "  padding: 9px 20px; border-radius: 6px 6px 0 0; margin-right: 3px;"
        "  font-size: 13px; }"
        "QTabBar::tab:selected { background: #7aa2f7; color: #0f0f17; font-weight: bold; }"
        "QTabBar::tab:hover:!selected { background: #24253a; color: #e2e4f0; }"

        // Кнопки — стандартная (синяя)
        "QPushButton { background: #7aa2f7; color: #0f0f17; border: none;"
        "  border-radius: 6px; padding: 7px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background: #a5bef8; }"
        "QPushButton:pressed { background: #5a82e0; }"
        "QPushButton:disabled { background: #2a2b40; color: #55566e; }"

        // Кнопки — красная (опасная)
        "QPushButton#btnDanger { background: #f7768e; color: #ffffff; font-weight: bold; }"
        "QPushButton#btnDanger:hover { background: #f99aab; }"
        "QPushButton#btnDanger:pressed { background: #e05070; }"

        // Кнопки — зелёная (успех)
        "QPushButton#btnSuccess { background: #73daca; color: #0f0f17; }"
        "QPushButton#btnSuccess:hover { background: #9ee8dc; }"
        "QPushButton#btnSuccess:pressed { background: #50c9b8; }"

        // Поля ввода
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
        "  background: #1a1b26; border: 1px solid #3b3d57; border-radius: 6px;"
        "  padding: 6px 10px; color: #e2e4f0; font-size: 13px; }"
        "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
        "  border: 1px solid #7aa2f7; background: #1e1f2e; }"
        "QLineEdit::placeholder { color: #55566e; }"
        "QSpinBox, QDoubleSpinBox { padding-right: 22px; }"
        "QSpinBox::up-button, QDoubleSpinBox::up-button {"
        "  subcontrol-origin: border; subcontrol-position: top right;"
        "  width: 22px; height: 50%;"
        "  border-left: 1px solid #3b3d57; border-bottom: 1px solid #3b3d57;"
        "  background: #1e2030; border-radius: 0 6px 0 0; }"
        "QSpinBox::down-button, QDoubleSpinBox::down-button {"
        "  subcontrol-origin: border; subcontrol-position: bottom right;"
        "  width: 22px; height: 50%;"
        "  border-left: 1px solid #3b3d57;"
        "  background: #1e2030; border-radius: 0 0 6px 0; }"
        "QSpinBox::up-button:hover, QDoubleSpinBox::up-button:hover {"
        "  background: #7aa2f7; }"
        "QSpinBox::down-button:hover, QDoubleSpinBox::down-button:hover {"
        "  background: #7aa2f7; }"
        "QSpinBox::up-arrow, QDoubleSpinBox::up-arrow {"
        "  width: 8px; height: 8px; }"
        "QSpinBox::down-arrow, QDoubleSpinBox::down-arrow {"
        "  width: 8px; height: 8px; }"
        "QComboBox::drop-down { border: none; padding-right: 8px; }"
        "QComboBox QAbstractItemView { background: #1a1b26; color: #e2e4f0;"
        "  selection-background-color: #2e3057; border: 1px solid #3b3d57; }"

        // Таблицы
        "QTableView { background: #13141f; border: 1px solid #3b3d57; border-radius: 6px;"
        "  gridline-color: #22233a; alternate-background-color: #191a29;"
        "  selection-background-color: #2e3057; color: #e2e4f0; }"
        "QTableView::item { padding: 4px 8px; color: #e2e4f0; }"
        "QTableView::item:selected { background: #2e3057; color: #ffffff; }"
        "QTableView::item:hover { background: #1e1f33; }"
        "QHeaderView::section { background: #1a1b26; color: #7aa2f7; padding: 7px 8px;"
        "  border: none; border-bottom: 2px solid #7aa2f7; font-weight: bold; font-size: 13px; }"
        "QHeaderView { background: #1a1b26; }"

        // Полосы прокрутки
        "QScrollBar:vertical { background: #13141f; width: 10px; border-radius: 5px; margin: 0; }"
        "QScrollBar::handle:vertical { background: #3b3d57; border-radius: 5px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: #7aa2f7; }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
        "QScrollBar:horizontal { background: #13141f; height: 10px; border-radius: 5px; }"
        "QScrollBar::handle:horizontal { background: #3b3d57; border-radius: 5px; }"
        "QScrollBar::handle:horizontal:hover { background: #7aa2f7; }"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"

        // Надписи
        "QLabel { color: #e2e4f0; background: transparent; }"
        "QLabel#labelTitle { font-size: 22px; font-weight: bold; color: #7aa2f7; }"
        "QLabel#labelSubtitle { color: #9899b3; font-size: 12px; }"

        // Группы и рамки
        "QGroupBox { border: 1px solid #3b3d57; border-radius: 8px; margin-top: 14px;"
        "  padding-top: 10px; background: #13141f; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px;"
        "  color: #7aa2f7; font-weight: bold; font-size: 13px; }"

        // Разделители (сплиттер)
        "QSplitter::handle { background: transparent; }"
        "QSplitter::handle:horizontal { width: 4px; }"
        "QSplitter::handle:vertical { height: 4px; }"
        "QSplitter::handle:hover { background: #7aa2f7; border-radius: 2px; }"

        // Статус-бар
        "QStatusBar { background: #0a0a12; color: #9899b3; border-top: 1px solid #3b3d57; }"
        "QStatusBar::item { border: none; }"

        // Диалоги и сообщения
        "QMessageBox { background: #1a1b26; }"
        "QMessageBox QLabel { color: #e2e4f0; font-size: 13px; }"
        "QMessageBox QPushButton { min-width: 80px; }"

        // Дата-редактор (для вкладок с фильтрами)
        "QDateEdit { background: #1a1b26; border: 1px solid #3b3d57; border-radius: 6px;"
        "  padding: 6px 10px; color: #e2e4f0; }"
        "QDateEdit:focus { border-color: #7aa2f7; }"
        "QCalendarWidget { background: #1a1b26; color: #e2e4f0; }"

        // Диалог ввода
        "QInputDialog QLabel { color: #e2e4f0; }"
        "QInputDialog QLineEdit { background: #1a1b26; border: 1px solid #3b3d57;"
        "  border-radius: 6px; padding: 6px; color: #e2e4f0; }"

        // Форма (FormLayout labels)
        "QFormLayout QLabel { color: #c0c2d8; }"
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