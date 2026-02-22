#include "ReportController.h"

QString ReportController::buildQuery(ReportType type, int shopId,
                                      const QString& from, const QString& to)
{
    switch (type) {
    case ReportType::MonthlySales:
        return QString(
            "SELECT to_char(DATE_TRUNC('month',o.created_at),'MM.YYYY') AS \"Месяц\","
            "  COUNT(*) AS \"Заказов\","
            "  SUM(o.total_amount) AS \"Выручка\","
            "  SUM(o.total_amount)-SUM(sub.cost) AS \"Прибыль\" "
            "FROM orders o "
            "JOIN LATERAL (SELECT SUM(oi.cost_price*oi.quantity) AS cost "
            "  FROM order_items oi WHERE oi.order_id=o.id) sub ON TRUE "
            "WHERE o.shop_id=%1 AND o.status='completed' "
            "  AND o.created_at::date BETWEEN '%2' AND '%3' "
            "GROUP BY DATE_TRUNC('month',o.created_at) ORDER BY 1 DESC"
        ).arg(shopId).arg(from).arg(to);

    case ReportType::Salaries:
        return QString(
            "SELECT e.full_name AS \"Сотрудник\",e.salary AS \"Оклад\","
            "  COALESCE(SUM(oi.sale_price*oi.quantity*oi.seller_pct),0) AS \"Комиссия\","
            "  e.salary+COALESCE(SUM(oi.sale_price*oi.quantity*oi.seller_pct),0) AS \"Итого\" "
            "FROM employees e "
            "LEFT JOIN orders o ON o.seller_id=e.id AND o.status='completed' "
            "  AND o.created_at::date BETWEEN '%2' AND '%3' "
            "LEFT JOIN order_items oi ON oi.order_id=o.id "
            "WHERE e.shop_id=%1 AND e.role='seller' AND e.is_active "
            "GROUP BY e.id,e.full_name,e.salary ORDER BY \"Итого\" DESC"
        ).arg(shopId).arg(from).arg(to);

    case ReportType::TopProducts:
        return QString(
            "SELECT p.name AS \"Товар\",oi.product_category AS \"Кат.\","
            "  SUM(oi.quantity) AS \"Продано\","
            "  SUM(oi.sale_price*oi.quantity) AS \"Выручка\","
            "  SUM((oi.sale_price-oi.cost_price)*oi.quantity) AS \"Прибыль\" "
            "FROM order_items oi JOIN products p ON p.id=oi.product_id "
            "JOIN orders o ON o.id=oi.order_id AND o.status='completed' "
            "WHERE oi.shop_id=%1 AND o.created_at::date BETWEEN '%2' AND '%3' "
            "GROUP BY p.name,oi.product_category ORDER BY \"Продано\" DESC LIMIT 20"
        ).arg(shopId).arg(from).arg(to);

    case ReportType::TopCategories:
        return QString(
            "SELECT oi.product_category AS \"Категория\","
            "  SUM(oi.quantity) AS \"Продано\","
            "  SUM(oi.sale_price*oi.quantity) AS \"Выручка\" "
            "FROM order_items oi "
            "JOIN orders o ON o.id=oi.order_id AND o.status='completed' "
            "WHERE oi.shop_id=%1 AND o.created_at::date BETWEEN '%2' AND '%3' "
            "GROUP BY oi.product_category ORDER BY \"Выручка\" DESC"
        ).arg(shopId).arg(from).arg(to);

    case ReportType::SalaryLog:
        return QString(
            "SELECT to_char(sl.changed_at,'DD.MM.YYYY HH24:MI') AS \"Дата\","
            "  e.full_name AS \"Сотрудник\","
            "  sl.old_salary AS \"Старый оклад\","
            "  sl.new_salary AS \"Новый оклад\","
            "  sl.new_salary-sl.old_salary AS \"Δ\","
            "  d.full_name AS \"Изменил\" "
            "FROM salary_log sl "
            "JOIN employees e ON e.id=sl.employee_id "
            "JOIN employees d ON d.id=sl.changed_by "
            "WHERE e.shop_id=%1 AND sl.changed_at::date BETWEEN '%2' AND '%3' "
            "ORDER BY sl.changed_at DESC"
        ).arg(shopId).arg(from).arg(to);

    case ReportType::StockLog:
        return QString(
            "SELECT to_char(stl.transferred_at,'DD.MM.YYYY HH24:MI') AS \"Дата\","
            "  p.name AS \"Товар\",stl.quantity AS \"Кол-во\","
            "  fw.name AS \"Откуда\",tw.name AS \"Куда\","
            "  e.full_name AS \"Инициатор\" "
            "FROM stock_transfer_log stl "
            "JOIN products p ON p.id=stl.product_id "
            "JOIN warehouses fw ON fw.id=stl.from_warehouse "
            "JOIN warehouses tw ON tw.id=stl.to_warehouse "
            "JOIN employees e ON e.id=stl.initiated_by "
            "WHERE tw.shop_id=%1 AND stl.transferred_at::date BETWEEN '%2' AND '%3' "
            "ORDER BY stl.transferred_at DESC"
        ).arg(shopId).arg(from).arg(to);
    }
    return {};
}
