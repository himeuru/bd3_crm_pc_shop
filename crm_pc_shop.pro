QT += core gui widgets sql

CONFIG += c++20
TARGET  = crm_pc_shop
TEMPLATE = app

SOURCES += \
    main.cpp \
    core/DatabaseManager.cpp \
    core/Session.cpp \
    core/AppConfig.cpp \
    repositories/AuthRepository.cpp \
    repositories/ProductRepository.cpp \
    repositories/OrderRepository.cpp \
    repositories/WarehouseRepository.cpp \
    repositories/EmployeeRepository.cpp \
    repositories/ClientRepository.cpp \
    repositories/ShopRepository.cpp \
    controllers/AuthController.cpp \
    controllers/OrderController.cpp \
    controllers/WarehouseController.cpp \
    controllers/StaffController.cpp \
    controllers/ReportController.cpp \
    ui/ConnectDialog.cpp \
    ui/LoginWindow.cpp \
    ui/Setupdialog.cpp \
    ui/widgets/SqlTableWidget.cpp \
    ui/director/DirectorMainWindow.cpp \
    ui/director/tabs/ShopsTab.cpp \
    ui/director/tabs/StaffTab.cpp \
    ui/director/tabs/OrdersTab.cpp \
    ui/director/tabs/WarehouseTab.cpp \
    ui/director/tabs/ReportsTab.cpp \
    ui/seller/SellerMainWindow.cpp \
    ui/seller/tabs/NewOrderTab.cpp \
    ui/seller/tabs/ClientsTab.cpp \
    ui/seller/tabs/StockTab.cpp \
    ui/seller/tabs/SalaryTab.cpp \
    ui/client/ClientMainWindow.cpp \
    ui/client/tabs/CatalogTab.cpp \
    ui/client/tabs/CartTab.cpp \
    ui/client/tabs/OrderHistoryTab.cpp \
    ui/client/tabs/ProfileTab.cpp

HEADERS += \
    core/DatabaseManager.h \
    core/Session.h \
    core/AppConfig.h \
    models/Shop.h \
    models/Product.h \
    models/Employee.h \
    models/Client.h \
    models/Order.h \
    models/StockItem.h \
    repositories/BaseRepository.h \
    repositories/AuthRepository.h \
    repositories/ProductRepository.h \
    repositories/OrderRepository.h \
    repositories/WarehouseRepository.h \
    repositories/EmployeeRepository.h \
    repositories/ClientRepository.h \
    repositories/ShopRepository.h \
    controllers/AuthController.h \
    controllers/OrderController.h \
    controllers/WarehouseController.h \
    controllers/StaffController.h \
    controllers/ReportController.h \
    ui/ConnectDialog.h \
    ui/LoginWindow.h \
    ui/Setupdialog.h \
    ui/widgets/SqlTableWidget.h \
    ui/director/DirectorMainWindow.h \
    ui/director/tabs/ShopsTab.h \
    ui/director/tabs/StaffTab.h \
    ui/director/tabs/OrdersTab.h \
    ui/director/tabs/WarehouseTab.h \
    ui/director/tabs/ReportsTab.h \
    ui/seller/SellerMainWindow.h \
    ui/seller/tabs/NewOrderTab.h \
    ui/seller/tabs/ClientsTab.h \
    ui/seller/tabs/StockTab.h \
    ui/seller/tabs/SalaryTab.h \
    ui/client/ClientMainWindow.h \
    ui/client/tabs/CatalogTab.h \
    ui/client/tabs/CartTab.h \
    ui/client/tabs/OrderHistoryTab.h \
    ui/client/tabs/ProfileTab.h
