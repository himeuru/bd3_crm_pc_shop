-- ============================================================
--  CRM PC Shop — Полная настройка базы данных PostgreSQL
--  Версия: финальная
--
--  Порядок выполнения:
--  1. Создать БД crm_pc_shop в pgAdmin
--  2. Подключиться к ней
--  3. Выполнить этот файл целиком (F5)
--
--  Если база уже существует — сначала сбросьте схему:
--    DROP SCHEMA public CASCADE;
--    CREATE SCHEMA public;
--    GRANT ALL ON SCHEMA public TO postgres;
--    GRANT USAGE ON SCHEMA public TO crm_app;
-- ============================================================


-- ============================================================
--  РАЗДЕЛ 0: Расширения и пользователь приложения
-- ============================================================

-- pgcrypto — хеширование паролей (bcrypt через crypt/gen_salt)
-- pg_trgm  — нечёткий поиск по триграммам
CREATE EXTENSION IF NOT EXISTS pgcrypto;
CREATE EXTENSION IF NOT EXISTS pg_trgm;

-- Отдельная роль для приложения — принцип минимальных прав.
-- Приложение никогда не подключается под postgres.
DO $$
BEGIN
  IF NOT EXISTS (SELECT FROM pg_roles WHERE rolname = 'crm_app') THEN
    CREATE ROLE crm_app LOGIN PASSWORD 'crm_secure_2025' CONNECTION LIMIT 50;
  END IF;
END$$;

GRANT CONNECT ON DATABASE crm_pc_shop TO crm_app;
GRANT USAGE   ON SCHEMA public TO crm_app;


-- ============================================================
--  РАЗДЕЛ 1: Таблицы
-- ============================================================

-- 1.1 Магазины (shops)
-- Корневая сущность. Все сотрудники, склады и заказы привязаны к магазину.
-- is_open = FALSE блокирует продажи через триггер trg_order_shop_open.
CREATE TABLE shops (
    id              SERIAL PRIMARY KEY,
    name            VARCHAR(100) NOT NULL,
    address         TEXT,
    is_open         BOOLEAN      NOT NULL DEFAULT TRUE,
    director_margin NUMERIC(5,4) NOT NULL DEFAULT 0.10
                    CHECK (director_margin >= 0 AND director_margin <= 1),
    created_at      TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);
CREATE UNIQUE INDEX idx_shops_name ON shops(lower(name));
CREATE INDEX        idx_shops_open  ON shops(id) WHERE is_open = TRUE;

-- 1.2 Склады (warehouses)
-- type='main' — один центральный склад (shop_id NULL).
-- type='micro' — один при каждом магазине. Списание идёт только с микросклада.
CREATE TABLE warehouses (
    id       SERIAL       PRIMARY KEY,
    name     VARCHAR(100) NOT NULL,
    type     VARCHAR(5)   NOT NULL CHECK (type IN ('main', 'micro')),
    shop_id  INTEGER      REFERENCES shops(id) ON DELETE RESTRICT,
    CONSTRAINT uq_micro_per_shop  UNIQUE (shop_id, type),
    CONSTRAINT chk_micro_has_shop CHECK (
        (type = 'micro' AND shop_id IS NOT NULL) OR
        (type = 'main'  AND shop_id IS NULL)
    )
);
CREATE INDEX idx_warehouses_shop ON warehouses(shop_id) WHERE shop_id IS NOT NULL;

-- 1.3 Товары (products)
-- search_vec — полнотекстовый вектор, обновляется автоматически при изменении полей.
CREATE TABLE products (
    id          SERIAL        PRIMARY KEY,
    name        VARCHAR(200)  NOT NULL,
    category    VARCHAR(100)  NOT NULL,
    description TEXT,
    cost_price  NUMERIC(12,2) NOT NULL CHECK (cost_price > 0),
    is_active   BOOLEAN       NOT NULL DEFAULT TRUE,
    created_at  TIMESTAMPTZ   NOT NULL DEFAULT NOW(),
    search_vec  TSVECTOR      GENERATED ALWAYS AS (
                    to_tsvector('russian',
                        name || ' ' || category || ' ' || COALESCE(description, ''))
                ) STORED
);
CREATE INDEX idx_products_active   ON products(id) WHERE is_active = TRUE;
CREATE INDEX idx_products_category ON products(category) WHERE is_active = TRUE;
CREATE INDEX idx_products_search   ON products USING GIN(search_vec);
CREATE INDEX idx_products_trgm     ON products USING GIN(lower(name) gin_trgm_ops);

-- 1.4 Остатки на складах (warehouse_stock)
-- Составной PK: (warehouse_id, product_id). quantity >= 0 — защита на уровне БД.
CREATE TABLE warehouse_stock (
    warehouse_id INTEGER       NOT NULL REFERENCES warehouses(id) ON DELETE RESTRICT,
    product_id   INTEGER       NOT NULL REFERENCES products(id)   ON DELETE RESTRICT,
    quantity     INTEGER       NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    updated_at   TIMESTAMPTZ   NOT NULL DEFAULT NOW(),
    PRIMARY KEY (warehouse_id, product_id)
);
CREATE INDEX idx_stock_product ON warehouse_stock(product_id);

-- 1.5 Сотрудники (employees)
-- CONSTRAINT uq_employees_shop_id нужен именно как CONSTRAINT (не индекс),
-- чтобы таблица orders могла ссылаться на (shop_id, id) через FOREIGN KEY.
-- Частичный уникальный индекс гарантирует одного активного директора на магазин.
CREATE TABLE employees (
    id            SERIAL        PRIMARY KEY,
    shop_id       INTEGER       NOT NULL REFERENCES shops(id) ON DELETE RESTRICT,
    full_name     VARCHAR(200)  NOT NULL,
    role          VARCHAR(10)   NOT NULL CHECK (role IN ('director', 'seller')),
    salary        NUMERIC(12,2) NOT NULL CHECK (salary >= 0),
    salary_pct    NUMERIC(5,4)  NOT NULL DEFAULT 0.10
                  CHECK (salary_pct >= 0 AND salary_pct <= 1),
    login         VARCHAR(100)  NOT NULL,
    password_hash TEXT          NOT NULL,
    is_active     BOOLEAN       NOT NULL DEFAULT TRUE,
    hired_at      TIMESTAMPTZ   NOT NULL DEFAULT NOW(),
    fired_at      TIMESTAMPTZ,
    CONSTRAINT chk_fired_after_hired CHECK (fired_at IS NULL OR fired_at > hired_at),
    CONSTRAINT uq_employees_shop_id  UNIQUE (shop_id, id)
);
CREATE UNIQUE INDEX idx_employees_login     ON employees(login);
CREATE UNIQUE INDEX idx_employees_shop_role ON employees(shop_id, role)
    WHERE role = 'director' AND is_active = TRUE;
CREATE INDEX        idx_employees_active_shop ON employees(shop_id) WHERE is_active = TRUE;

-- 1.6 Клиенты (clients)
-- Отдельная таблица от сотрудников. discount_pct (0-50%) — персональная скидка.
CREATE TABLE clients (
    id            SERIAL        PRIMARY KEY,
    full_name     VARCHAR(200)  NOT NULL,
    phone         VARCHAR(30),
    email         VARCHAR(150),
    login         VARCHAR(100)  NOT NULL,
    password_hash TEXT          NOT NULL,
    discount_pct  NUMERIC(5,4)  NOT NULL DEFAULT 0.00
                  CHECK (discount_pct >= 0 AND discount_pct <= 0.5),
    registered_at TIMESTAMPTZ   NOT NULL DEFAULT NOW()
);
CREATE UNIQUE INDEX idx_clients_login ON clients(login);
CREATE INDEX        idx_clients_phone ON clients(phone) WHERE phone IS NOT NULL;

-- 1.7 Заказы (orders)
-- FK (shop_id, seller_id) гарантирует что продавец принадлежит тому же магазину.
-- chk_cancelled_consistency: у отменённого заказа обязательно заполнены cancelled_by и cancelled_at.
-- uq_orders_shop_id нужен как CONSTRAINT для FK в order_items.
CREATE TABLE orders (
    id           SERIAL        PRIMARY KEY,
    shop_id      INTEGER       NOT NULL,
    seller_id    INTEGER       NOT NULL,
    client_id    INTEGER       NOT NULL REFERENCES clients(id) ON DELETE RESTRICT,
    status       VARCHAR(12)   NOT NULL DEFAULT 'completed'
                 CHECK (status IN ('completed', 'cancelled')),
    discount_pct NUMERIC(5,4)  NOT NULL DEFAULT 0.00,
    total_amount NUMERIC(14,2) NOT NULL CHECK (total_amount >= 0),
    cancelled_by INTEGER       REFERENCES employees(id),
    created_at   TIMESTAMPTZ   NOT NULL DEFAULT NOW(),
    cancelled_at TIMESTAMPTZ,
    CONSTRAINT fk_order_seller FOREIGN KEY (shop_id, seller_id)
        REFERENCES employees(shop_id, id),
    CONSTRAINT chk_cancelled_consistency CHECK (
        (status = 'cancelled' AND cancelled_by IS NOT NULL AND cancelled_at IS NOT NULL) OR
        (status = 'completed' AND cancelled_by IS NULL     AND cancelled_at IS NULL)
    ),
    CONSTRAINT uq_orders_shop_id UNIQUE (shop_id, id)
);
CREATE INDEX idx_orders_shop      ON orders(shop_id, created_at DESC);
CREATE INDEX idx_orders_seller    ON orders(seller_id, created_at DESC);
CREATE INDEX idx_orders_client    ON orders(client_id);
CREATE INDEX idx_orders_created   ON orders USING BRIN(created_at);
CREATE INDEX idx_orders_cancelled ON orders(shop_id) WHERE status = 'cancelled';

-- 1.8 Позиции заказа (order_items)
-- Денормализация: product_category и shop_id хранятся здесь для быстрых отчётов.
-- cost_price и sale_price — снимки цен на момент продажи (неизменны).
-- DEFERRABLE INITIALLY DEFERRED: FK проверяется в конце транзакции.
CREATE TABLE order_items (
    order_id         INTEGER       NOT NULL REFERENCES orders(id)   ON DELETE RESTRICT,
    product_id       INTEGER       NOT NULL REFERENCES products(id)  ON DELETE RESTRICT,
    shop_id          INTEGER       NOT NULL,
    product_category VARCHAR(100)  NOT NULL,
    quantity         INTEGER       NOT NULL CHECK (quantity > 0),
    cost_price       NUMERIC(12,2) NOT NULL,
    sale_price       NUMERIC(12,2) NOT NULL,
    seller_pct       NUMERIC(5,4)  NOT NULL,
    director_margin  NUMERIC(5,4)  NOT NULL,
    PRIMARY KEY (order_id, product_id),
    CONSTRAINT fk_item_shop_order FOREIGN KEY (shop_id, order_id)
        REFERENCES orders(shop_id, id) DEFERRABLE INITIALLY DEFERRED
);
CREATE INDEX idx_items_shop_cat ON order_items(shop_id, product_category);
CREATE INDEX idx_items_product  ON order_items(product_id);

-- 1.9 Журнал изменений зарплат (salary_log)
-- Каждое изменение оклада — неизменяемая запись.
CREATE TABLE salary_log (
    id          SERIAL        PRIMARY KEY,
    employee_id INTEGER       NOT NULL REFERENCES employees(id),
    changed_by  INTEGER       NOT NULL REFERENCES employees(id),
    old_salary  NUMERIC(12,2) NOT NULL,
    new_salary  NUMERIC(12,2) NOT NULL,
    changed_at  TIMESTAMPTZ   NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_salary_log ON salary_log USING BRIN(changed_at);

-- 1.10 Журнал переводов между складами (stock_transfer_log)
-- Каждый вызов replenish_micro_warehouse() пишет сюда по одной строке на товар.
CREATE TABLE stock_transfer_log (
    id             SERIAL      PRIMARY KEY,
    product_id     INTEGER     NOT NULL REFERENCES products(id),
    from_warehouse INTEGER     REFERENCES warehouses(id),
    to_warehouse   INTEGER     REFERENCES warehouses(id),
    quantity       INTEGER     NOT NULL CHECK (quantity > 0),
    initiated_by   INTEGER     NOT NULL REFERENCES employees(id),
    transferred_at TIMESTAMPTZ NOT NULL DEFAULT NOW()
);
CREATE INDEX idx_transfer_log ON stock_transfer_log USING BRIN(transferred_at);


-- ============================================================
--  РАЗДЕЛ 2: Функции
-- ============================================================

-- 2.1 calc_sale_price — расчёт цены продажи с наценкой и скидкой
-- Формула: себестоимость × (1 + фиксированная наценка 80% + % продавца + % директора)
--          × (1 - скидка клиента)
-- IMMUTABLE: PostgreSQL кеширует результат, вызов параллелен.
CREATE OR REPLACE FUNCTION calc_sale_price(
    p_cost_price   NUMERIC,
    p_seller_pct   NUMERIC,
    p_director_pct NUMERIC,
    p_discount_pct NUMERIC
) RETURNS NUMERIC AS $$
BEGIN
    RETURN ROUND(
        p_cost_price
        * (1.0 + 0.50 + p_seller_pct + 0.20 + 0.10 + p_director_pct)
        * (1.0 - p_discount_pct), 2);
END;
$$ LANGUAGE plpgsql IMMUTABLE PARALLEL SAFE;

-- 2.2 authenticate_user — аутентификация
-- Ищет сначала в employees (директор/продавец), затем в clients.
-- SECURITY DEFINER: приложение не трогает password_hash напрямую.
CREATE OR REPLACE FUNCTION authenticate_user(p_login VARCHAR, p_password VARCHAR)
RETURNS TABLE(user_id INTEGER, user_role VARCHAR, shop_id INTEGER, full_name VARCHAR) AS $$
BEGIN
    RETURN QUERY
        SELECT e.id, e.role::VARCHAR, e.shop_id, e.full_name
        FROM employees e
        WHERE e.login = p_login
          AND e.password_hash = crypt(p_password, e.password_hash)
          AND e.is_active = TRUE;
    IF NOT FOUND THEN
        RETURN QUERY
            SELECT c.id, 'client'::VARCHAR, NULL::INTEGER, c.full_name
            FROM clients c
            WHERE c.login = p_login
              AND c.password_hash = crypt(p_password, c.password_hash);
    END IF;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- 2.3 create_order — создание заказа (атомарная операция)
-- Проверяет наличие каждого товара на микроскладе (FOR UPDATE — защита от гонки).
-- При нехватке товара — RAISE EXCEPTION откатывает всю транзакцию.
CREATE OR REPLACE FUNCTION create_order(
    p_shop_id INTEGER, p_client_id INTEGER, p_seller_id INTEGER, p_items JSONB
) RETURNS INTEGER AS $$
DECLARE
    v_order_id INTEGER; v_discount_pct NUMERIC; v_seller_pct NUMERIC;
    v_dir_margin NUMERIC; v_micro_wh INTEGER; v_item JSONB;
    v_product_id INTEGER; v_qty INTEGER; v_cost NUMERIC; v_sale NUMERIC;
    v_total NUMERIC := 0; v_stock INTEGER; v_category VARCHAR;
BEGIN
    SELECT discount_pct INTO STRICT v_discount_pct FROM clients WHERE id = p_client_id;
    SELECT e.salary_pct, s.director_margin INTO STRICT v_seller_pct, v_dir_margin
    FROM employees e JOIN shops s ON s.id = p_shop_id
    WHERE e.id = p_seller_id AND e.shop_id = p_shop_id AND e.is_active = TRUE;
    SELECT id INTO STRICT v_micro_wh FROM warehouses WHERE shop_id = p_shop_id AND type = 'micro';
    INSERT INTO orders (shop_id, seller_id, client_id, discount_pct, total_amount, status)
    VALUES (p_shop_id, p_seller_id, p_client_id, v_discount_pct, 0, 'completed')
    RETURNING id INTO v_order_id;
    FOR v_item IN SELECT * FROM jsonb_array_elements(p_items) LOOP
        v_product_id := (v_item->>'product_id')::INTEGER;
        v_qty        := (v_item->>'qty')::INTEGER;
        SELECT quantity INTO v_stock FROM warehouse_stock
        WHERE warehouse_id = v_micro_wh AND product_id = v_product_id FOR UPDATE;
        IF v_stock IS NULL OR v_stock < v_qty THEN
            RAISE EXCEPTION 'INSUFFICIENT_STOCK:product_id=%:available=%',
                v_product_id, COALESCE(v_stock, 0);
        END IF;
        SELECT cost_price, category INTO STRICT v_cost, v_category FROM products WHERE id = v_product_id;
        v_sale  := calc_sale_price(v_cost, v_seller_pct, v_dir_margin, v_discount_pct);
        v_total := v_total + (v_sale * v_qty);
        INSERT INTO order_items
            (order_id, product_id, shop_id, product_category, quantity, cost_price, sale_price, seller_pct, director_margin)
        VALUES (v_order_id, v_product_id, p_shop_id, v_category, v_qty, v_cost, v_sale, v_seller_pct, v_dir_margin);
        UPDATE warehouse_stock SET quantity = quantity - v_qty
        WHERE warehouse_id = v_micro_wh AND product_id = v_product_id;
    END LOOP;
    UPDATE orders SET total_amount = v_total WHERE id = v_order_id;
    RETURN v_order_id;
END;
$$ LANGUAGE plpgsql;

-- 2.4 cancel_order — отмена заказа (только директор своего магазина)
-- При отмене возвращает товары на микросклад.
CREATE OR REPLACE FUNCTION cancel_order(p_order_id INTEGER, p_director_id INTEGER)
RETURNS VOID AS $$
DECLARE v_order RECORD; v_micro_wh INTEGER;
BEGIN
    PERFORM 1 FROM employees WHERE id = p_director_id AND role = 'director' AND is_active = TRUE;
    IF NOT FOUND THEN RAISE EXCEPTION 'ACCESS_DENIED: only active director can cancel orders'; END IF;
    SELECT * INTO STRICT v_order FROM orders WHERE id = p_order_id;
    IF v_order.status = 'cancelled' THEN
        RAISE EXCEPTION 'ORDER_ALREADY_CANCELLED: order_id=%', p_order_id; END IF;
    PERFORM 1 FROM employees WHERE id = p_director_id AND shop_id = v_order.shop_id;
    IF NOT FOUND THEN RAISE EXCEPTION 'ACCESS_DENIED: director does not manage this shop'; END IF;
    SELECT id INTO STRICT v_micro_wh FROM warehouses WHERE shop_id = v_order.shop_id AND type = 'micro';
    INSERT INTO warehouse_stock (warehouse_id, product_id, quantity)
    SELECT v_micro_wh, product_id, quantity FROM order_items WHERE order_id = p_order_id
    ON CONFLICT (warehouse_id, product_id)
    DO UPDATE SET quantity = warehouse_stock.quantity + EXCLUDED.quantity, updated_at = NOW();
    UPDATE orders SET status = 'cancelled', cancelled_by = p_director_id, cancelled_at = NOW()
    WHERE id = p_order_id;
END;
$$ LANGUAGE plpgsql;

-- 2.5 replenish_micro_warehouse — пополнение микросклада из главного
-- Для каждого товара вычисляет нехватку до p_fill_to, переносит не больше чем есть.
-- Каждый перевод записывается в stock_transfer_log.
CREATE OR REPLACE FUNCTION replenish_micro_warehouse(
    p_shop_id INTEGER, p_initiated_by INTEGER, p_fill_to INTEGER DEFAULT 50
) RETURNS VOID AS $$
DECLARE
    v_micro_wh INTEGER; v_main_wh INTEGER; v_rec RECORD;
    v_current INTEGER; v_need INTEGER; v_transfer INTEGER;
BEGIN
    SELECT id INTO STRICT v_micro_wh FROM warehouses WHERE shop_id = p_shop_id AND type = 'micro';
    SELECT id INTO STRICT v_main_wh  FROM warehouses WHERE type = 'main' LIMIT 1;
    FOR v_rec IN
        SELECT product_id, quantity AS main_qty FROM warehouse_stock
        WHERE warehouse_id = v_main_wh AND quantity > 0
    LOOP
        SELECT COALESCE(quantity, 0) INTO v_current FROM warehouse_stock
        WHERE warehouse_id = v_micro_wh AND product_id = v_rec.product_id;
        v_need     := p_fill_to - v_current;
        v_transfer := LEAST(v_need, v_rec.main_qty);
        CONTINUE WHEN v_transfer <= 0;
        UPDATE warehouse_stock SET quantity = quantity - v_transfer
        WHERE warehouse_id = v_main_wh AND product_id = v_rec.product_id;
        INSERT INTO warehouse_stock (warehouse_id, product_id, quantity)
        VALUES (v_micro_wh, v_rec.product_id, v_transfer)
        ON CONFLICT (warehouse_id, product_id)
        DO UPDATE SET quantity = warehouse_stock.quantity + v_transfer, updated_at = NOW();
        INSERT INTO stock_transfer_log
            (product_id, from_warehouse, to_warehouse, quantity, initiated_by)
        VALUES (v_rec.product_id, v_main_wh, v_micro_wh, v_transfer, p_initiated_by);
    END LOOP;
END;
$$ LANGUAGE plpgsql;

-- 2.6 change_password — смена пароля (сотрудник или клиент)
-- Требует подтверждение старого пароля. Возвращает TRUE при успехе.
CREATE OR REPLACE FUNCTION change_password(
    p_user_id INTEGER, p_role VARCHAR, p_old_pass VARCHAR, p_new_pass VARCHAR
) RETURNS BOOLEAN AS $$
DECLARE v_rows INTEGER;
BEGIN
    IF p_role = 'employee' THEN
        UPDATE employees SET password_hash = crypt(p_new_pass, gen_salt('bf', 12))
        WHERE id = p_user_id AND password_hash = crypt(p_old_pass, password_hash) AND is_active = TRUE;
    ELSIF p_role = 'client' THEN
        UPDATE clients SET password_hash = crypt(p_new_pass, gen_salt('bf', 12))
        WHERE id = p_user_id AND password_hash = crypt(p_old_pass, password_hash);
    ELSE RETURN FALSE; END IF;
    GET DIAGNOSTICS v_rows = ROW_COUNT;
    RETURN v_rows > 0;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;

-- 2.7 hire_employee — найм сотрудника
-- Защита: нельзя нанять второго директора в один магазин.
CREATE OR REPLACE FUNCTION hire_employee(
    p_shop_id INTEGER, p_full_name VARCHAR, p_role VARCHAR,
    p_salary NUMERIC, p_salary_pct NUMERIC, p_login VARCHAR, p_password VARCHAR
) RETURNS INTEGER AS $$
DECLARE v_id INTEGER;
BEGIN
    IF p_role = 'director' THEN
        PERFORM 1 FROM employees WHERE shop_id = p_shop_id AND role = 'director' AND is_active = TRUE;
        IF FOUND THEN RAISE EXCEPTION 'DIRECTOR_EXISTS: shop already has an active director'; END IF;
    END IF;
    INSERT INTO employees (shop_id, full_name, role, salary, salary_pct, login, password_hash)
    VALUES (p_shop_id, p_full_name, p_role, p_salary, p_salary_pct,
            p_login, crypt(p_password, gen_salt('bf', 12)))
    RETURNING id INTO v_id;
    RETURN v_id;
END;
$$ LANGUAGE plpgsql;

-- 2.8 fire_employee — увольнение (мягкое удаление)
-- Только директор своего магазина. Данные и история сохраняются.
CREATE OR REPLACE FUNCTION fire_employee(p_employee_id INTEGER, p_director_id INTEGER)
RETURNS VOID AS $$
DECLARE v_emp RECORD;
BEGIN
    SELECT * INTO STRICT v_emp FROM employees WHERE id = p_employee_id;
    PERFORM 1 FROM employees WHERE id = p_director_id AND shop_id = v_emp.shop_id
        AND role = 'director' AND is_active = TRUE;
    IF NOT FOUND THEN RAISE EXCEPTION 'ACCESS_DENIED: you are not director of this shop'; END IF;
    IF NOT v_emp.is_active THEN RAISE EXCEPTION 'ALREADY_FIRED: employee_id=%', p_employee_id; END IF;
    UPDATE employees SET is_active = FALSE, fired_at = NOW() WHERE id = p_employee_id;
END;
$$ LANGUAGE plpgsql;

-- 2.9 update_salary — изменение оклада с логированием
-- Записывает в salary_log до обновления.
CREATE OR REPLACE FUNCTION update_salary(
    p_employee_id INTEGER, p_new_salary NUMERIC, p_director_id INTEGER
) RETURNS VOID AS $$
DECLARE v_emp RECORD;
BEGIN
    SELECT * INTO STRICT v_emp FROM employees WHERE id = p_employee_id;
    PERFORM 1 FROM employees WHERE id = p_director_id AND shop_id = v_emp.shop_id
        AND role = 'director' AND is_active = TRUE;
    IF NOT FOUND THEN RAISE EXCEPTION 'ACCESS_DENIED'; END IF;
    INSERT INTO salary_log (employee_id, changed_by, old_salary, new_salary)
    VALUES (p_employee_id, p_director_id, v_emp.salary, p_new_salary);
    UPDATE employees SET salary = p_new_salary WHERE id = p_employee_id;
END;
$$ LANGUAGE plpgsql;

-- 2.10 Триггер: блокировка продаж в закрытом магазине
-- Срабатывает BEFORE INSERT на orders. Если магазин закрыт — INSERT отменяется.
CREATE OR REPLACE FUNCTION trg_fn_block_sale_closed_shop() RETURNS TRIGGER AS $$
BEGIN
    PERFORM 1 FROM shops WHERE id = NEW.shop_id AND is_open = TRUE;
    IF NOT FOUND THEN RAISE EXCEPTION 'SHOP_CLOSED: shop_id=%', NEW.shop_id; END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_order_shop_open
BEFORE INSERT ON orders
FOR EACH ROW EXECUTE FUNCTION trg_fn_block_sale_closed_shop();

-- 2.11 Триггер: автообновление updated_at на складе
CREATE OR REPLACE FUNCTION trg_fn_update_stock_ts() RETURNS TRIGGER AS $$
BEGIN NEW.updated_at = NOW(); RETURN NEW; END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_stock_updated_at
BEFORE UPDATE ON warehouse_stock
FOR EACH ROW EXECUTE FUNCTION trg_fn_update_stock_ts();


-- ============================================================
--  РАЗДЕЛ 3: Представления (VIEW)
-- ============================================================

-- Полный обзор остатков на всех складах
CREATE OR REPLACE VIEW v_stock_overview AS
SELECT w.id AS warehouse_id, w.type AS warehouse_type,
    s.id AS shop_id, s.name AS shop_name,
    p.id AS product_id, p.name AS product_name,
    p.category, ws.quantity, p.cost_price, ws.updated_at
FROM warehouse_stock ws
JOIN warehouses  w ON w.id = ws.warehouse_id
LEFT JOIN shops  s ON s.id = w.shop_id
JOIN products    p ON p.id = ws.product_id;

-- Заработок продавцов по месяцам: оклад + комиссия
CREATE OR REPLACE VIEW v_seller_earnings_monthly AS
SELECT e.id AS employee_id, e.full_name, e.shop_id, e.salary AS base_salary,
    DATE_TRUNC('month', o.created_at) AS month, COUNT(o.id) AS orders_count,
    COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0) AS commission,
    e.salary + COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0) AS total_earnings
FROM employees e
LEFT JOIN orders      o  ON o.seller_id = e.id AND o.status = 'completed'
LEFT JOIN order_items oi ON oi.order_id = o.id
WHERE e.role = 'seller'
GROUP BY e.id, e.full_name, e.shop_id, e.salary, month;

-- Топ товаров по продажам (только завершённые заказы)
CREATE OR REPLACE VIEW v_top_products AS
SELECT oi.product_id, p.name AS product_name, oi.product_category,
    SUM(oi.quantity) AS total_sold,
    SUM(oi.sale_price * oi.quantity) AS total_revenue,
    SUM((oi.sale_price - oi.cost_price) * oi.quantity) AS total_profit
FROM order_items oi
JOIN products p ON p.id = oi.product_id
JOIN orders   o ON o.id = oi.order_id AND o.status = 'completed'
GROUP BY oi.product_id, p.name, oi.product_category;


-- ============================================================
--  РАЗДЕЛ 4: Права доступа для пользователя приложения
-- ============================================================
GRANT SELECT, INSERT, UPDATE ON ALL TABLES    IN SCHEMA public TO crm_app;
GRANT EXECUTE                 ON ALL FUNCTIONS IN SCHEMA public TO crm_app;
GRANT USAGE, SELECT           ON ALL SEQUENCES IN SCHEMA public TO crm_app;

ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT SELECT, INSERT, UPDATE ON TABLES    TO crm_app;
ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT EXECUTE                ON FUNCTIONS TO crm_app;


-- ============================================================
--  РАЗДЕЛ 5: Тестовые данные
--  Порядок важен: 5.1 → 5.2 → 5.3 → 5.4 → 5.5 → 5.6 → 5.7 → 5.8 → 5.9
-- ============================================================

-- 5.1 Магазины (5 городов с разной маржой директора)
INSERT INTO shops (name, address, director_margin) VALUES
    ('PC Shop Москва',          'ул. Тверская, 1',        0.10),
    ('PC Shop Санкт-Петербург', 'Невский пр., 50',         0.12),
    ('PC Shop Казань',          'ул. Баумана, 20',          0.08),
    ('PC Shop Новосибирск',     'Красный проспект, 15',    0.09),
    ('PC Shop Екатеринбург',    'ул. Ленина, 42',           0.11);

-- 5.2 Склады (1 главный + 5 микроскладов при каждом магазине)
INSERT INTO warehouses (name, type, shop_id) VALUES
    ('Центральный склад',       'main',  NULL),
    ('Микросклад Москва',       'micro', 1),
    ('Микросклад СПб',          'micro', 2),
    ('Микросклад Казань',       'micro', 3),
    ('Микросклад Новосибирск',  'micro', 4),
    ('Микросклад Екатеринбург', 'micro', 5);

-- 5.3 Товары (57 позиций в 9 категориях: CPU, GPU, RAM, SSD, PSU, MB, COOL, HDD, CASE, PERIPH, MONITOR)
INSERT INTO products (name, category, description, cost_price) VALUES
    ('Intel Core i9-14900K',           'CPU', 'Флагманский процессор Intel, 24 ядра, до 6.0 ГГц',    35000),
    ('Intel Core i7-14700K',           'CPU', 'Производительный процессор Intel, 20 ядер',            22000),
    ('Intel Core i5-13600K',           'CPU', 'Оптимальный выбор для игр, 14 ядер',                   18000),
    ('Intel Core i3-13100F',           'CPU', 'Бюджетный процессор Intel, 4 ядра',                    7500),
    ('AMD Ryzen 9 7950X',              'CPU', 'Флагман AMD, 16 ядер, 32 потока, до 5.7 ГГц',          40000),
    ('AMD Ryzen 7 7700X',              'CPU', 'Игровой процессор AMD, 8 ядер, 16 потоков',            22000),
    ('AMD Ryzen 5 7600X',              'CPU', 'Бюджетный игровой процессор AMD, 6 ядер',              12000),
    ('AMD Ryzen 5 5600X',              'CPU', 'Популярный процессор предыдущего поколения',            9000),
    ('NVIDIA GeForce RTX 4090',        'GPU', 'Флагманская видеокарта 24GB GDDR6X',                  120000),
    ('NVIDIA GeForce RTX 4080 Super',  'GPU', 'Топовая видеокарта 16GB GDDR6X',                       80000),
    ('NVIDIA GeForce RTX 4070 Ti',     'GPU', 'Высокопроизводительная карта 12GB GDDR6X',             55000),
    ('NVIDIA GeForce RTX 4060 Ti',     'GPU', 'Оптимальный выбор 1440p, 8GB GDDR6',                   30000),
    ('AMD Radeon RX 7900 XTX',         'GPU', 'Флагман AMD, 24GB GDDR6',                              70000),
    ('AMD Radeon RX 7800 XT',          'GPU', 'Игровая карта AMD 1440p, 16GB GDDR6',                  35000),
    ('AMD Radeon RX 7600',             'GPU', 'Бюджетная карта AMD 1080p, 8GB GDDR6',                 18000),
    ('Corsair Vengeance 32GB DDR5',    'RAM', '2x16GB DDR5-6000 CL36, RGB подсветка',                 12000),
    ('Kingston Fury Beast 32GB DDR5',  'RAM', '2x16GB DDR5-5200 CL40',                                10000),
    ('G.Skill Trident Z5 64GB DDR5',   'RAM', '2x32GB DDR5-6400 CL32, для рабочих станций',           25000),
    ('Crucial 16GB DDR4',              'RAM', '2x8GB DDR4-3200, бюджетная планка',                     4500),
    ('Kingston Fury 16GB DDR5',        'RAM', '2x8GB DDR5-5200',                                       6000),
    ('Samsung 990 Pro 2TB NVMe',       'SSD', 'PCIe 4.0, чтение до 7450 MB/s',                       15000),
    ('WD Black SN850X 1TB',            'SSD', 'PCIe 4.0, чтение до 7300 MB/s',                        9000),
    ('Seagate FireCuda 530 2TB',       'SSD', 'PCIe 4.0, чтение до 7300 MB/s',                       14000),
    ('Kingston NV2 1TB',               'SSD', 'PCIe 4.0, бюджетный вариант',                          4000),
    ('Crucial MX500 2TB SATA',         'SSD', 'SATA SSD, надёжность и доступность',                   6000),
    ('Seasonic Prime 1000W Gold',      'PSU', '80+ Gold, полностью модульный, 10 лет гарантии',        8000),
    ('Corsair RM850x',                 'PSU', '80+ Gold, 850W, тихий вентилятор',                     7000),
    ('be quiet! Straight Power 750W',  'PSU', '80+ Platinum, 750W, Zero RPM режим',                   9000),
    ('Thermaltake Smart 600W',         'PSU', '80+ White, 600W, бюджетный вариант',                   3500),
    ('ASUS ROG Maximus Z790 Hero',     'MB',  'Топовая материнка Intel Z790 ATX, DDR5',               45000),
    ('MSI MAG B760 TOMAHAWK',          'MB',  'Надёжная плата Intel B760 ATX, DDR5',                  15000),
    ('Gigabyte B650 AORUS Elite AX',   'MB',  'Плата AMD B650 ATX с Wi-Fi 6E',                        20000),
    ('MSI MAG B650 TOMAHAWK',          'MB',  'Надёжная материнка AMD B650 ATX',                       18000),
    ('ASRock B550 Phantom Gaming 4',   'MB',  'Доступная плата AMD B550 ATX',                          8000),
    ('Noctua NH-D15',                  'COOL','Лучший воздушный кулер, 2x140mm',                       6500),
    ('be quiet! Dark Rock Pro 4',      'COOL','Тихий мощный кулер, TDP 250W',                          5500),
    ('Corsair iCUE H150i Elite',       'COOL','360mm AIO СЖО, RGB, 3x120mm',                           9000),
    ('DeepCool AK620',                 'COOL','Двухбашенный кулер, TDP 260W',                          3500),
    ('Seagate Barracuda 4TB HDD',      'HDD', 'SATA 6Gb/s, 5400 RPM, кэш 256MB',                     4500),
    ('WD Blue 2TB HDD',                'HDD', 'SATA 3.5", 7200 RPM, надёжный и тихий',                3200),
    ('Toshiba X300 6TB HDD',           'HDD', 'Для хранения данных, 7200 RPM',                        6800),
    ('NZXT H510 Flow',                 'CASE','Mid-Tower ATX, боковое стекло, 2x140mm вентилятора',   7500),
    ('Fractal Design Meshify C',       'CASE','Compact ATX, отличная вентиляция',                     8500),
    ('Lian Li PC-O11 Dynamic',         'CASE','Full-Tower, двойная камера, для СЖО',                 13000),
    ('be quiet! Pure Base 500DX',      'CASE','Mid-Tower ATX, звукоизоляция, ARGB',                  10000),
    ('Logitech G502 HERO',             'PERIPH','Игровая мышь 25600 DPI, 11 кнопок',                  4800),
    ('Logitech MX Keys',               'PERIPH','Беспроводная клавиатура, подсветка',                  8500),
    ('Razer BlackWidow V3',            'PERIPH','Механическая клавиатура, Green switches',              9000),
    ('SteelSeries Arctis Nova Pro',    'PERIPH','Беспроводные наушники, шумоподавление',              18000),
    ('Logitech C920 HD Pro',           'PERIPH','Веб-камера 1080p, автофокус, стерео',                 5500),
    ('Samsung Odyssey G5 27"',         'MONITOR','27" QHD 165Hz, VA, 1ms MPRT, HDR10',               22000),
    ('LG 27GP850-B 27"',               'MONITOR','27" QHD 165Hz, IPS, Nano IPS, G-Sync',             28000),
    ('AOC 24G2 24"',                   'MONITOR','24" FHD 144Hz, IPS, FreeSync, 1ms',                12000),
    ('ASUS ROG Swift PG279QM 27"',     'MONITOR','27" QHD 240Hz, IPS, G-Sync Ultimate',              55000),
    ('Cooler Master Hyper 212 Black',  'COOL', 'Одна башня, 120mm вентилятор, TDP 180W',              2200),
    ('Arctic Freezer 34 eSports',      'COOL', 'Двухбашенный, 120mm BioniX, TDP 210W',                2800),
    ('EKWB EK-AIO 240mm',              'COOL', '240mm СЖО, высококачественная помпа',                  9500),
    ('Thermal Grizzly Kryonaut 5.5g',  'COOL', 'Термопаста премиум, теплопроводность 12.5',            800);

-- 5.4 Заполнить главный склад — 500 штук каждого товара
INSERT INTO warehouse_stock (warehouse_id, product_id, quantity)
SELECT 1, id, 500 FROM products;

-- 5.5 Сотрудники (5 директоров + 9 продавцов)
-- Директора: Dir2025!   Продавцы: Sell2025!
INSERT INTO employees (shop_id, full_name, role, salary, salary_pct, login, password_hash) VALUES
    (1, 'Иванов Алексей Петрович',     'director', 120000, 0.10, 'director_msk', crypt('Dir2025!',  gen_salt('bf', 12))),
    (2, 'Смирнова Елена Ивановна',     'director', 110000, 0.10, 'director_spb', crypt('Dir2025!',  gen_salt('bf', 12))),
    (3, 'Галиев Рустам Маратович',     'director', 100000, 0.10, 'director_kzn', crypt('Dir2025!',  gen_salt('bf', 12))),
    (4, 'Белова Ирина Сергеевна',      'director', 105000, 0.10, 'director_nsk', crypt('Dir2025!',  gen_salt('bf', 12))),
    (5, 'Орлов Максим Владимирович',   'director', 108000, 0.10, 'director_ekb', crypt('Dir2025!',  gen_salt('bf', 12))),
    (1, 'Петров Сергей Николаевич',    'seller',    45000, 0.10, 'seller_msk_1', crypt('Sell2025!', gen_salt('bf', 12))),
    (1, 'Козлова Анна Дмитриевна',     'seller',    42000, 0.10, 'seller_msk_2', crypt('Sell2025!', gen_salt('bf', 12))),
    (1, 'Романов Артём Игоревич',      'seller',    43000, 0.10, 'seller_msk_3', crypt('Sell2025!', gen_salt('bf', 12))),
    (2, 'Николаев Виктор Олегович',    'seller',    43000, 0.10, 'seller_spb_1', crypt('Sell2025!', gen_salt('bf', 12))),
    (2, 'Федотова Валерия Павловна',   'seller',    41000, 0.10, 'seller_spb_2', crypt('Sell2025!', gen_salt('bf', 12))),
    (3, 'Хасанов Дамир Ренатович',     'seller',    40000, 0.10, 'seller_kzn_1', crypt('Sell2025!', gen_salt('bf', 12))),
    (3, 'Зиятдинова Алина Ильгизовна', 'seller',    39000, 0.10, 'seller_kzn_2', crypt('Sell2025!', gen_salt('bf', 12))),
    (4, 'Захаров Павел Андреевич',     'seller',    41000, 0.10, 'seller_nsk_1', crypt('Sell2025!', gen_salt('bf', 12))),
    (5, 'Власова Наталья Юрьевна',     'seller',    42000, 0.10, 'seller_ekb_1', crypt('Sell2025!', gen_salt('bf', 12)));

-- 5.6 Клиенты (12 человек, скидки от 0% до 15%)
-- Все пароли: pass123
INSERT INTO clients (full_name, phone, email, login, password_hash, discount_pct) VALUES
    ('Сидорова Мария Александровна',    '+79001234567', 'sidorova@mail.ru',    'client_001', crypt('pass123', gen_salt('bf', 12)), 0.05),
    ('Волков Дмитрий Сергеевич',        '+79009876543', 'volkov@gmail.com',    'client_002', crypt('pass123', gen_salt('bf', 12)), 0.03),
    ('Новикова Юлия Андреевна',         '+79005553322', 'novikova@ya.ru',      'client_003', crypt('pass123', gen_salt('bf', 12)), 0.10),
    ('Морозов Игорь Валентинович',      '+79004441100', NULL,                   'client_004', crypt('pass123', gen_salt('bf', 12)), 0.00),
    ('Лебедева Ксения Игоревна',        '+79007772255', 'lebedeva@mail.ru',    'client_005', crypt('pass123', gen_salt('bf', 12)), 0.07),
    ('Тихонов Роман Евгеньевич',        '+79003331122', 'tikhonov@inbox.ru',   'client_006', crypt('pass123', gen_salt('bf', 12)), 0.02),
    ('Шевцова Дарья Олеговна',          '+79008884433', 'shevtsova@bk.ru',     'client_007', crypt('pass123', gen_salt('bf', 12)), 0.05),
    ('Куликов Антон Вячеславович',      '+79002226677', 'kulikov@gmail.com',    'client_008', crypt('pass123', gen_salt('bf', 12)), 0.00),
    ('Рыжова Светлана Николаевна',      '+79006665544', 'ryzhova@mail.ru',     'client_009', crypt('pass123', gen_salt('bf', 12)), 0.08),
    ('Макаров Виталий Дмитриевич',      '+79001117788', 'makarov@ya.ru',       'client_010', crypt('pass123', gen_salt('bf', 12)), 0.15),
    ('Громова Екатерина Андреевна',     '+79005559900', NULL,                   'client_011', crypt('pass123', gen_salt('bf', 12)), 0.03),
    ('Соловьёв Алексей Константинович', '+79004448811', 'soloviev@inbox.ru',   'client_012', crypt('pass123', gen_salt('bf', 12)), 0.05);

-- 5.7 Заполнить все микросклады напрямую (150 штук каждого товара)
-- Используем прямую вставку, а не replenish_micro_warehouse,
-- чтобы не опустошать главный склад при инициализации.
INSERT INTO warehouse_stock (warehouse_id, product_id, quantity)
SELECT w.id, p.id, 150
FROM warehouses w CROSS JOIN products p
WHERE w.type = 'micro'
ON CONFLICT (warehouse_id, product_id) DO UPDATE SET quantity = 150;

-- 5.8 Тестовые заказы (42 для Москвы + 7 для других магазинов)
-- Магазины 1 (Москва) — продавцы id=6,7,8. Заказы будут распределены по 4 месяцам.
SELECT create_order(1,  1, 6, '[{"product_id":1,"qty":1},{"product_id":16,"qty":2},{"product_id":21,"qty":1}]'::jsonb);
SELECT create_order(1,  2, 7, '[{"product_id":9,"qty":1}]'::jsonb);
SELECT create_order(1,  3, 8, '[{"product_id":3,"qty":1},{"product_id":17,"qty":1},{"product_id":26,"qty":1}]'::jsonb);
SELECT create_order(1,  4, 6, '[{"product_id":11,"qty":1},{"product_id":22,"qty":1}]'::jsonb);
SELECT create_order(1,  5, 7, '[{"product_id":5,"qty":1},{"product_id":18,"qty":1}]'::jsonb);
SELECT create_order(1,  6, 8, '[{"product_id":35,"qty":1},{"product_id":30,"qty":1}]'::jsonb);
SELECT create_order(1,  7, 6, '[{"product_id":13,"qty":1},{"product_id":27,"qty":1}]'::jsonb);
SELECT create_order(1,  8, 7, '[{"product_id":2,"qty":1},{"product_id":19,"qty":2},{"product_id":24,"qty":1}]'::jsonb);
SELECT create_order(1,  9, 8, '[{"product_id":6,"qty":1},{"product_id":31,"qty":1}]'::jsonb);
SELECT create_order(1, 10, 6, '[{"product_id":4,"qty":2},{"product_id":20,"qty":1},{"product_id":29,"qty":1}]'::jsonb);
SELECT create_order(1,  1, 7, '[{"product_id":10,"qty":1},{"product_id":23,"qty":1}]'::jsonb);
SELECT create_order(1,  2, 8, '[{"product_id":7,"qty":1},{"product_id":28,"qty":1},{"product_id":36,"qty":1}]'::jsonb);
SELECT create_order(1,  3, 6, '[{"product_id":14,"qty":1},{"product_id":32,"qty":1}]'::jsonb);
SELECT create_order(1,  4, 7, '[{"product_id":12,"qty":1},{"product_id":25,"qty":1},{"product_id":37,"qty":1}]'::jsonb);
SELECT create_order(1,  5, 8, '[{"product_id":8,"qty":1},{"product_id":33,"qty":1}]'::jsonb);
SELECT create_order(1,  6, 6, '[{"product_id":15,"qty":1},{"product_id":34,"qty":1},{"product_id":38,"qty":1}]'::jsonb);
SELECT create_order(1,  7, 7, '[{"product_id":1,"qty":1},{"product_id":21,"qty":2}]'::jsonb);
SELECT create_order(1,  8, 8, '[{"product_id":9,"qty":1},{"product_id":16,"qty":1}]'::jsonb);
SELECT create_order(1,  9, 6, '[{"product_id":5,"qty":1},{"product_id":22,"qty":1}]'::jsonb);
SELECT create_order(1, 10, 7, '[{"product_id":11,"qty":1},{"product_id":27,"qty":1}]'::jsonb);
SELECT create_order(1, 11, 8, '[{"product_id":3,"qty":1},{"product_id":30,"qty":1}]'::jsonb);
SELECT create_order(1, 12, 6, '[{"product_id":13,"qty":1},{"product_id":26,"qty":1},{"product_id":35,"qty":1}]'::jsonb);
SELECT create_order(1,  1, 7, '[{"product_id":2,"qty":1},{"product_id":17,"qty":2},{"product_id":24,"qty":1}]'::jsonb);
SELECT create_order(1,  2, 8, '[{"product_id":6,"qty":1},{"product_id":31,"qty":1}]'::jsonb);
SELECT create_order(1,  3, 6, '[{"product_id":10,"qty":1},{"product_id":19,"qty":1},{"product_id":29,"qty":1}]'::jsonb);
SELECT create_order(1,  4, 7, '[{"product_id":4,"qty":1},{"product_id":20,"qty":2}]'::jsonb);
SELECT create_order(1,  5, 8, '[{"product_id":7,"qty":1},{"product_id":28,"qty":1}]'::jsonb);
SELECT create_order(1,  6, 6, '[{"product_id":14,"qty":1},{"product_id":32,"qty":1},{"product_id":37,"qty":1}]'::jsonb);
SELECT create_order(1,  7, 7, '[{"product_id":12,"qty":1},{"product_id":25,"qty":1}]'::jsonb);
SELECT create_order(1,  8, 8, '[{"product_id":8,"qty":1},{"product_id":33,"qty":1},{"product_id":38,"qty":1}]'::jsonb);
SELECT create_order(1,  9, 6, '[{"product_id":15,"qty":1},{"product_id":34,"qty":1}]'::jsonb);
SELECT create_order(1, 10, 7, '[{"product_id":1,"qty":1},{"product_id":23,"qty":1},{"product_id":16,"qty":1}]'::jsonb);
SELECT create_order(1,  1, 8, '[{"product_id":9,"qty":1},{"product_id":22,"qty":1}]'::jsonb);
SELECT create_order(1,  2, 6, '[{"product_id":5,"qty":1},{"product_id":18,"qty":2},{"product_id":36,"qty":1}]'::jsonb);
SELECT create_order(1,  3, 7, '[{"product_id":11,"qty":1},{"product_id":27,"qty":1}]'::jsonb);
SELECT create_order(1,  4, 8, '[{"product_id":3,"qty":1},{"product_id":30,"qty":1},{"product_id":21,"qty":1}]'::jsonb);
SELECT create_order(1,  5, 6, '[{"product_id":13,"qty":1},{"product_id":26,"qty":1}]'::jsonb);
SELECT create_order(1,  6, 7, '[{"product_id":2,"qty":1},{"product_id":17,"qty":1},{"product_id":35,"qty":1}]'::jsonb);
SELECT create_order(1,  7, 8, '[{"product_id":6,"qty":1},{"product_id":31,"qty":1}]'::jsonb);
SELECT create_order(1,  8, 6, '[{"product_id":10,"qty":1},{"product_id":29,"qty":1}]'::jsonb);
SELECT create_order(1,  9, 7, '[{"product_id":7,"qty":1},{"product_id":28,"qty":1},{"product_id":20,"qty":1}]'::jsonb);
SELECT create_order(1, 10, 8, '[{"product_id":4,"qty":1},{"product_id":19,"qty":1}]'::jsonb);
-- Другие магазины
SELECT create_order(2,  1, 9, '[{"product_id":2,"qty":1},{"product_id":17,"qty":1}]'::jsonb);
SELECT create_order(2,  3,10, '[{"product_id":9,"qty":1},{"product_id":21,"qty":1}]'::jsonb);
SELECT create_order(2,  5, 9, '[{"product_id":11,"qty":1},{"product_id":27,"qty":1}]'::jsonb);
SELECT create_order(3,  2,11, '[{"product_id":7,"qty":1},{"product_id":36,"qty":1}]'::jsonb);
SELECT create_order(3,  4,11, '[{"product_id":5,"qty":1},{"product_id":22,"qty":1}]'::jsonb);
SELECT create_order(4,  6,13, '[{"product_id":4,"qty":1},{"product_id":19,"qty":1}]'::jsonb);
SELECT create_order(5,  8,14, '[{"product_id":15,"qty":1},{"product_id":31,"qty":1}]'::jsonb);

-- 5.9 Сдвинуть даты заказов Москвы на 4 разных месяца (для отчётов по периодам)
WITH ranked AS (
    SELECT id, ROW_NUMBER() OVER (ORDER BY id DESC) AS rn
    FROM orders WHERE shop_id = 1
)
UPDATE orders SET created_at =
    CASE
        WHEN rn BETWEEN  1 AND 10 THEN NOW() - INTERVAL '3 days'  + (random()*INTERVAL '2 days')
        WHEN rn BETWEEN 11 AND 20 THEN NOW() - INTERVAL '35 days' + (random()*INTERVAL '25 days')
        WHEN rn BETWEEN 21 AND 30 THEN NOW() - INTERVAL '65 days' + (random()*INTERVAL '25 days')
        WHEN rn BETWEEN 31 AND 42 THEN NOW() - INTERVAL '95 days' + (random()*INTERVAL '25 days')
        ELSE created_at
    END
FROM ranked WHERE orders.id = ranked.id;

-- 5.10 Изменения зарплат (заполняет журнал salary_log)
SELECT update_salary(6,  48000, 1);
SELECT update_salary(7,  44000, 1);
SELECT update_salary(8,  45000, 1);
SELECT update_salary(6,  50000, 1);
SELECT update_salary(7,  46000, 1);
SELECT update_salary(8,  48000, 1);
SELECT update_salary(9,  45000, 2);
SELECT update_salary(11, 42000, 3);

-- ============================================================
--  ГОТОВО! Итоговая статистика:
--
--  Магазинов:   5 (Москва, СПб, Казань, Новосибирск, Екатеринбург)
--  Складов:     6 (1 главный + 5 микроскладов)
--  Товаров:    57 (CPU, GPU, RAM, SSD, PSU, MB, COOL, HDD, CASE, PERIPH, MONITOR)
--  Сотрудников: 14 (5 директоров + 9 продавцов)
--  Клиентов:    12 (скидки 0–15%)
--  Заказов:     49 (42 для Москвы с датами за 4 месяца + 7 для других магазинов)
--  salary_log:  8 записей
--
--  Логины для входа:
--  Директор Москвы:  director_msk / Dir2025!
--  Директор СПб:     director_spb / Dir2025!
--  Продавец Москвы:  seller_msk_1 / Sell2025!
--  Клиент (5%):      client_001   / pass123
--  Клиент (15%):     client_010   / pass123
-- ============================================================
