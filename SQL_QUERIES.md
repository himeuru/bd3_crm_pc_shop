# SQL — Создание, заполнение и тестирование БД
## CRM PC Shop (PostgreSQL)

> Запускать блоки строго по порядку разделов.  
> Каждый блок можно скопировать целиком в pgAdmin Query Tool и выполнить через **F5**.

---

## 0. Подготовка — создать БД и пользователя

```sql
CREATE DATABASE crm_pc_shop
    ENCODING    'UTF8'
    LC_COLLATE  'ru_RU.UTF-8'
    LC_CTYPE    'ru_RU.UTF-8'
    TEMPLATE    template0;
```

> Следующие команды выполнять уже внутри базы `crm_pc_shop`.

```sql
CREATE EXTENSION IF NOT EXISTS pgcrypto;
CREATE EXTENSION IF NOT EXISTS pg_trgm;
```

```sql
CREATE ROLE crm_app
    LOGIN
    PASSWORD 'crm_secure_2025'
    CONNECTION LIMIT 50;

GRANT CONNECT ON DATABASE crm_pc_shop TO crm_app;
GRANT USAGE   ON SCHEMA public TO crm_app;
```

---

## 1. Создание таблиц

### 1.1 Магазины

```sql
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
```

### 1.2 Склады

```sql
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
```

### 1.3 Товары

```sql
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
```

### 1.4 Остатки на складах

```sql
CREATE TABLE warehouse_stock (
    warehouse_id INTEGER       NOT NULL REFERENCES warehouses(id) ON DELETE RESTRICT,
    product_id   INTEGER       NOT NULL REFERENCES products(id)   ON DELETE RESTRICT,
    quantity     INTEGER       NOT NULL DEFAULT 0 CHECK (quantity >= 0),
    updated_at   TIMESTAMPTZ   NOT NULL DEFAULT NOW(),
    PRIMARY KEY (warehouse_id, product_id)
);

CREATE INDEX idx_stock_product ON warehouse_stock(product_id);
```

### 1.5 Сотрудники

```sql
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
    CONSTRAINT chk_fired_after_hired CHECK (fired_at IS NULL OR fired_at > hired_at)
);

CREATE UNIQUE INDEX idx_employees_login
    ON employees USING HASH(login);

CREATE UNIQUE INDEX idx_employees_shop_role
    ON employees(shop_id, role)
    WHERE role = 'director' AND is_active = TRUE;

CREATE INDEX idx_employees_active_shop
    ON employees(shop_id)
    WHERE is_active = TRUE;

CREATE UNIQUE INDEX idx_employees_shop_seller
    ON employees(shop_id, id)
    WHERE role = 'seller' AND is_active = TRUE;
```

### 1.6 Клиенты

```sql
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

CREATE UNIQUE INDEX idx_clients_login ON clients USING HASH(login);
CREATE INDEX        idx_clients_phone ON clients(phone) WHERE phone IS NOT NULL;
```

### 1.7 Заказы

```sql
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
    )
);

CREATE INDEX idx_orders_shop      ON orders(shop_id, created_at DESC);
CREATE INDEX idx_orders_seller    ON orders(seller_id, created_at DESC);
CREATE INDEX idx_orders_client    ON orders(client_id);
CREATE INDEX idx_orders_created   ON orders USING BRIN(created_at);
CREATE INDEX idx_orders_cancelled ON orders(shop_id) WHERE status = 'cancelled';
```

### 1.8 Позиции заказа

```sql
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
```

### 1.9 Журнал зарплат

```sql
CREATE TABLE salary_log (
    id          SERIAL        PRIMARY KEY,
    employee_id INTEGER       NOT NULL REFERENCES employees(id),
    changed_by  INTEGER       NOT NULL REFERENCES employees(id),
    old_salary  NUMERIC(12,2) NOT NULL,
    new_salary  NUMERIC(12,2) NOT NULL,
    changed_at  TIMESTAMPTZ   NOT NULL DEFAULT NOW()
);

CREATE INDEX idx_salary_log ON salary_log USING BRIN(changed_at);
```

### 1.10 Журнал переводов между складами

```sql
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
```

---

## 2. Функции и триггеры

### 2.1 Расчёт цены продажи

```sql
CREATE OR REPLACE FUNCTION calc_sale_price(
    p_cost_price   NUMERIC,
    p_seller_pct   NUMERIC,
    p_director_pct NUMERIC,
    p_discount_pct NUMERIC
)
RETURNS NUMERIC AS $$
BEGIN
    RETURN ROUND(
        p_cost_price
        * (1.0 + 0.50 + p_seller_pct + 0.20 + 0.10 + p_director_pct)
        * (1.0 - p_discount_pct),
        2
    );
END;
$$ LANGUAGE plpgsql IMMUTABLE PARALLEL SAFE;
```

### 2.2 Аутентификация пользователя

```sql
CREATE OR REPLACE FUNCTION authenticate_user(
    p_login    VARCHAR,
    p_password VARCHAR
)
RETURNS TABLE(
    user_id   INTEGER,
    user_role VARCHAR,
    shop_id   INTEGER,
    full_name VARCHAR
) AS $$
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
```

### 2.3 Создание заказа (атомарное)

```sql
CREATE OR REPLACE FUNCTION create_order(
    p_shop_id   INTEGER,
    p_client_id INTEGER,
    p_seller_id INTEGER,
    p_items     JSONB
)
RETURNS INTEGER AS $$
DECLARE
    v_order_id     INTEGER;
    v_discount_pct NUMERIC;
    v_seller_pct   NUMERIC;
    v_dir_margin   NUMERIC;
    v_micro_wh     INTEGER;
    v_item         JSONB;
    v_product_id   INTEGER;
    v_qty          INTEGER;
    v_cost         NUMERIC;
    v_sale         NUMERIC;
    v_total        NUMERIC := 0;
    v_stock        INTEGER;
    v_category     VARCHAR;
BEGIN
    SELECT discount_pct INTO STRICT v_discount_pct
    FROM clients WHERE id = p_client_id;

    SELECT e.salary_pct, s.director_margin
    INTO STRICT v_seller_pct, v_dir_margin
    FROM employees e
    JOIN shops s ON s.id = p_shop_id
    WHERE e.id = p_seller_id AND e.shop_id = p_shop_id AND e.is_active = TRUE;

    SELECT id INTO STRICT v_micro_wh
    FROM warehouses WHERE shop_id = p_shop_id AND type = 'micro';

    INSERT INTO orders
        (shop_id, seller_id, client_id, discount_pct, total_amount, status)
    VALUES
        (p_shop_id, p_seller_id, p_client_id, v_discount_pct, 0, 'completed')
    RETURNING id INTO v_order_id;

    FOR v_item IN SELECT * FROM jsonb_array_elements(p_items) LOOP
        v_product_id := (v_item->>'product_id')::INTEGER;
        v_qty        := (v_item->>'qty')::INTEGER;

        SELECT quantity INTO v_stock
        FROM warehouse_stock
        WHERE warehouse_id = v_micro_wh AND product_id = v_product_id
        FOR UPDATE;

        IF v_stock IS NULL OR v_stock < v_qty THEN
            RAISE EXCEPTION 'INSUFFICIENT_STOCK:product_id=%:available=%',
                v_product_id, COALESCE(v_stock, 0);
        END IF;

        SELECT cost_price, category INTO STRICT v_cost, v_category
        FROM products WHERE id = v_product_id;

        v_sale  := calc_sale_price(v_cost, v_seller_pct, v_dir_margin, v_discount_pct);
        v_total := v_total + (v_sale * v_qty);

        INSERT INTO order_items
            (order_id, product_id, shop_id, product_category,
             quantity, cost_price, sale_price, seller_pct, director_margin)
        VALUES
            (v_order_id, v_product_id, p_shop_id, v_category,
             v_qty, v_cost, v_sale, v_seller_pct, v_dir_margin);

        UPDATE warehouse_stock
        SET quantity = quantity - v_qty
        WHERE warehouse_id = v_micro_wh AND product_id = v_product_id;
    END LOOP;

    UPDATE orders SET total_amount = v_total WHERE id = v_order_id;

    RETURN v_order_id;
END;
$$ LANGUAGE plpgsql;
```

### 2.4 Отмена заказа (только директор)

```sql
CREATE OR REPLACE FUNCTION cancel_order(
    p_order_id    INTEGER,
    p_director_id INTEGER
)
RETURNS VOID AS $$
DECLARE
    v_order    RECORD;
    v_micro_wh INTEGER;
BEGIN
    PERFORM 1 FROM employees
    WHERE id = p_director_id AND role = 'director' AND is_active = TRUE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'ACCESS_DENIED: only active director can cancel orders';
    END IF;

    SELECT * INTO STRICT v_order FROM orders WHERE id = p_order_id;

    IF v_order.status = 'cancelled' THEN
        RAISE EXCEPTION 'ORDER_ALREADY_CANCELLED: order_id=%', p_order_id;
    END IF;

    PERFORM 1 FROM employees
    WHERE id = p_director_id AND shop_id = v_order.shop_id;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'ACCESS_DENIED: director does not manage this shop';
    END IF;

    SELECT id INTO STRICT v_micro_wh
    FROM warehouses WHERE shop_id = v_order.shop_id AND type = 'micro';

    INSERT INTO warehouse_stock (warehouse_id, product_id, quantity)
    SELECT v_micro_wh, product_id, quantity
    FROM order_items WHERE order_id = p_order_id
    ON CONFLICT (warehouse_id, product_id)
    DO UPDATE SET quantity   = warehouse_stock.quantity + EXCLUDED.quantity,
                  updated_at = NOW();

    UPDATE orders
    SET status       = 'cancelled',
        cancelled_by = p_director_id,
        cancelled_at = NOW()
    WHERE id = p_order_id;
END;
$$ LANGUAGE plpgsql;
```

### 2.5 Пополнение микросклада из главного

```sql
CREATE OR REPLACE FUNCTION replenish_micro_warehouse(
    p_shop_id      INTEGER,
    p_initiated_by INTEGER,
    p_fill_to      INTEGER DEFAULT 50
)
RETURNS VOID AS $$
DECLARE
    v_micro_wh INTEGER;
    v_main_wh  INTEGER;
    v_rec      RECORD;
    v_current  INTEGER;
    v_need     INTEGER;
    v_transfer INTEGER;
BEGIN
    SELECT id INTO STRICT v_micro_wh
    FROM warehouses WHERE shop_id = p_shop_id AND type = 'micro';

    SELECT id INTO STRICT v_main_wh
    FROM warehouses WHERE type = 'main' LIMIT 1;

    FOR v_rec IN
        SELECT product_id, quantity AS main_qty
        FROM warehouse_stock
        WHERE warehouse_id = v_main_wh AND quantity > 0
    LOOP
        SELECT COALESCE(quantity, 0) INTO v_current
        FROM warehouse_stock
        WHERE warehouse_id = v_micro_wh AND product_id = v_rec.product_id;

        v_need     := p_fill_to - v_current;
        v_transfer := LEAST(v_need, v_rec.main_qty);

        CONTINUE WHEN v_transfer <= 0;

        UPDATE warehouse_stock
        SET quantity = quantity - v_transfer
        WHERE warehouse_id = v_main_wh AND product_id = v_rec.product_id;

        INSERT INTO warehouse_stock (warehouse_id, product_id, quantity)
        VALUES (v_micro_wh, v_rec.product_id, v_transfer)
        ON CONFLICT (warehouse_id, product_id)
        DO UPDATE SET quantity   = warehouse_stock.quantity + v_transfer,
                      updated_at = NOW();

        INSERT INTO stock_transfer_log
            (product_id, from_warehouse, to_warehouse, quantity, initiated_by)
        VALUES
            (v_rec.product_id, v_main_wh, v_micro_wh, v_transfer, p_initiated_by);
    END LOOP;
END;
$$ LANGUAGE plpgsql;
```

### 2.6 Смена пароля

```sql
CREATE OR REPLACE FUNCTION change_password(
    p_user_id  INTEGER,
    p_role     VARCHAR,
    p_old_pass VARCHAR,
    p_new_pass VARCHAR
)
RETURNS BOOLEAN AS $$
DECLARE
    v_rows INTEGER;
BEGIN
    IF p_role = 'employee' THEN
        UPDATE employees
        SET password_hash = crypt(p_new_pass, gen_salt('bf', 12))
        WHERE id = p_user_id
          AND password_hash = crypt(p_old_pass, password_hash)
          AND is_active = TRUE;
    ELSIF p_role = 'client' THEN
        UPDATE clients
        SET password_hash = crypt(p_new_pass, gen_salt('bf', 12))
        WHERE id = p_user_id
          AND password_hash = crypt(p_old_pass, password_hash);
    ELSE
        RETURN FALSE;
    END IF;
    GET DIAGNOSTICS v_rows = ROW_COUNT;
    RETURN v_rows > 0;
END;
$$ LANGUAGE plpgsql SECURITY DEFINER;
```

### 2.7 Нанять сотрудника

```sql
CREATE OR REPLACE FUNCTION hire_employee(
    p_shop_id    INTEGER,
    p_full_name  VARCHAR,
    p_role       VARCHAR,
    p_salary     NUMERIC,
    p_salary_pct NUMERIC,
    p_login      VARCHAR,
    p_password   VARCHAR
)
RETURNS INTEGER AS $$
DECLARE
    v_id INTEGER;
BEGIN
    IF p_role = 'director' THEN
        PERFORM 1 FROM employees
        WHERE shop_id = p_shop_id AND role = 'director' AND is_active = TRUE;
        IF FOUND THEN
            RAISE EXCEPTION 'DIRECTOR_EXISTS: shop already has an active director';
        END IF;
    END IF;

    INSERT INTO employees
        (shop_id, full_name, role, salary, salary_pct, login, password_hash)
    VALUES
        (p_shop_id, p_full_name, p_role, p_salary, p_salary_pct,
         p_login, crypt(p_password, gen_salt('bf', 12)))
    RETURNING id INTO v_id;

    RETURN v_id;
END;
$$ LANGUAGE plpgsql;
```

### 2.8 Уволить сотрудника

```sql
CREATE OR REPLACE FUNCTION fire_employee(
    p_employee_id INTEGER,
    p_director_id INTEGER
)
RETURNS VOID AS $$
DECLARE
    v_emp RECORD;
BEGIN
    SELECT * INTO STRICT v_emp FROM employees WHERE id = p_employee_id;

    PERFORM 1 FROM employees
    WHERE id = p_director_id
      AND shop_id = v_emp.shop_id
      AND role = 'director'
      AND is_active = TRUE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'ACCESS_DENIED: you are not director of this shop';
    END IF;

    IF NOT v_emp.is_active THEN
        RAISE EXCEPTION 'ALREADY_FIRED: employee_id=%', p_employee_id;
    END IF;

    UPDATE employees SET is_active = FALSE, fired_at = NOW()
    WHERE id = p_employee_id;
END;
$$ LANGUAGE plpgsql;
```

### 2.9 Изменить зарплату (с логированием)

```sql
CREATE OR REPLACE FUNCTION update_salary(
    p_employee_id INTEGER,
    p_new_salary  NUMERIC,
    p_director_id INTEGER
)
RETURNS VOID AS $$
DECLARE
    v_emp RECORD;
BEGIN
    SELECT * INTO STRICT v_emp FROM employees WHERE id = p_employee_id;

    PERFORM 1 FROM employees
    WHERE id = p_director_id
      AND shop_id = v_emp.shop_id
      AND role = 'director'
      AND is_active = TRUE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'ACCESS_DENIED';
    END IF;

    INSERT INTO salary_log (employee_id, changed_by, old_salary, new_salary)
    VALUES (p_employee_id, p_director_id, v_emp.salary, p_new_salary);

    UPDATE employees SET salary = p_new_salary WHERE id = p_employee_id;
END;
$$ LANGUAGE plpgsql;
```

### 2.10 Триггер — блокировка продаж в закрытом магазине

```sql
CREATE OR REPLACE FUNCTION trg_fn_block_sale_closed_shop()
RETURNS TRIGGER AS $$
BEGIN
    PERFORM 1 FROM shops WHERE id = NEW.shop_id AND is_open = TRUE;
    IF NOT FOUND THEN
        RAISE EXCEPTION 'SHOP_CLOSED: shop_id=%', NEW.shop_id;
    END IF;
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_order_shop_open
BEFORE INSERT ON orders
FOR EACH ROW EXECUTE FUNCTION trg_fn_block_sale_closed_shop();
```

### 2.11 Триггер — автообновление updated_at на складе

```sql
CREATE OR REPLACE FUNCTION trg_fn_update_stock_ts()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_stock_updated_at
BEFORE UPDATE ON warehouse_stock
FOR EACH ROW EXECUTE FUNCTION trg_fn_update_stock_ts();
```

---

## 3. Представления (VIEW)

```sql
CREATE OR REPLACE VIEW v_stock_overview AS
SELECT
    w.id          AS warehouse_id,
    w.type        AS warehouse_type,
    s.id          AS shop_id,
    s.name        AS shop_name,
    p.id          AS product_id,
    p.name        AS product_name,
    p.category,
    ws.quantity,
    p.cost_price,
    ws.updated_at
FROM warehouse_stock ws
JOIN warehouses  w ON w.id = ws.warehouse_id
LEFT JOIN shops  s ON s.id = w.shop_id
JOIN products    p ON p.id = ws.product_id;
```

```sql
CREATE OR REPLACE VIEW v_seller_earnings_monthly AS
SELECT
    e.id                                                             AS employee_id,
    e.full_name,
    e.shop_id,
    e.salary                                                         AS base_salary,
    DATE_TRUNC('month', o.created_at)                               AS month,
    COUNT(o.id)                                                      AS orders_count,
    COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0)   AS commission,
    e.salary + COALESCE(SUM(oi.sale_price * oi.quantity * oi.seller_pct), 0) AS total_earnings
FROM employees e
LEFT JOIN orders      o  ON o.seller_id = e.id AND o.status = 'completed'
LEFT JOIN order_items oi ON oi.order_id = o.id
WHERE e.role = 'seller'
GROUP BY e.id, e.full_name, e.shop_id, e.salary, month;
```

```sql
CREATE OR REPLACE VIEW v_top_products AS
SELECT
    oi.product_id,
    p.name                                                    AS product_name,
    oi.product_category,
    SUM(oi.quantity)                                          AS total_sold,
    SUM(oi.sale_price * oi.quantity)                          AS total_revenue,
    SUM((oi.sale_price - oi.cost_price) * oi.quantity)        AS total_profit
FROM order_items oi
JOIN products p ON p.id = oi.product_id
JOIN orders   o ON o.id = oi.order_id AND o.status = 'completed'
GROUP BY oi.product_id, p.name, oi.product_category;
```

---

## 4. Права доступа для пользователя приложения

```sql
GRANT SELECT, INSERT, UPDATE ON ALL TABLES IN SCHEMA public TO crm_app;
GRANT EXECUTE ON ALL FUNCTIONS IN SCHEMA public TO crm_app;
GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA public TO crm_app;

ALTER DEFAULT PRIVILEGES IN SCHEMA public
    GRANT SELECT, INSERT, UPDATE ON TABLES TO crm_app;
ALTER DEFAULT PRIVILEGES IN SCHEMA public
    GRANT EXECUTE ON FUNCTIONS TO crm_app;
```

---

## 5. Заполнение тестовыми данными

### 5.1 Магазины

```sql
INSERT INTO shops (name, address, director_margin) VALUES
    ('PC Shop Москва',          'ул. Тверская, 1',   0.10),
    ('PC Shop Санкт-Петербург', 'Невский пр., 50',   0.12),
    ('PC Shop Казань',          'ул. Баумана, 20',    0.08);
```

### 5.2 Склады

```sql
INSERT INTO warehouses (name, type, shop_id) VALUES
    ('Центральный склад', 'main',  NULL),
    ('Микросклад Москва', 'micro', 1),
    ('Микросклад СПб',    'micro', 2),
    ('Микросклад Казань', 'micro', 3);
```

### 5.3 Товары (14 позиций)

```sql
INSERT INTO products (name, category, description, cost_price) VALUES
    ('Intel Core i9-14900K',        'CPU', 'Флагманский процессор Intel 24 ядра',         35000),
    ('AMD Ryzen 9 7950X',           'CPU', 'Флагман AMD 16 ядер 32 потока',               40000),
    ('Intel Core i5-13600K',        'CPU', 'Оптимальный выбор для игр, 14 ядер',          18000),
    ('AMD Ryzen 5 7600X',           'CPU', 'Бюджетный игровой процессор AMD',             12000),
    ('NVIDIA GeForce RTX 4090',     'GPU', 'Топовая видеокарта 24GB GDDR6X',             120000),
    ('NVIDIA GeForce RTX 4070 Ti',  'GPU', 'Высокопроизводительная карта 12GB',          55000),
    ('AMD Radeon RX 7900 XTX',      'GPU', 'Флагман AMD 24GB GDDR6',                     70000),
    ('Corsair Vengeance 32GB DDR5', 'RAM', '2x16GB DDR5-6000 CL36',                      12000),
    ('Kingston Fury 16GB DDR5',     'RAM', '2x8GB DDR5-5200',                              6000),
    ('Samsung 990 Pro 2TB NVMe',    'SSD', 'PCIe 4.0 до 7450 MB/s',                      15000),
    ('WD Black SN850X 1TB',         'SSD', 'PCIe 4.0 до 7300 MB/s',                       9000),
    ('Seasonic Prime 1000W Gold',   'PSU', '80+ Gold полностью модульный',                 8000),
    ('ASUS ROG Maximus Z790',       'MB',  'Топовая материнка Intel Z790 ATX',            45000),
    ('MSI MAG B650 TOMAHAWK',       'MB',  'Надёжная материнка AMD B650 ATX',             18000);
```

### 5.4 Заполнить главный склад

```sql
INSERT INTO warehouse_stock (warehouse_id, product_id, quantity)
SELECT 1, id, 200 FROM products;
```

### 5.5 Сотрудники

```sql
INSERT INTO employees (shop_id, full_name, role, salary, salary_pct, login, password_hash) VALUES
    (1, 'Иванов Алексей Петрович',   'director', 120000, 0.10,
        'director_msk', crypt('Dir2025!',  gen_salt('bf', 12))),
    (2, 'Смирнова Елена Ивановна',   'director', 110000, 0.10,
        'director_spb', crypt('Dir2025!',  gen_salt('bf', 12))),
    (3, 'Галиев Рустам Маратович',   'director', 100000, 0.10,
        'director_kzn', crypt('Dir2025!',  gen_salt('bf', 12))),
    (1, 'Петров Сергей Николаевич',  'seller',    45000, 0.10,
        'seller_msk_1', crypt('Sell2025!', gen_salt('bf', 12))),
    (1, 'Козлова Анна Дмитриевна',   'seller',    42000, 0.10,
        'seller_msk_2', crypt('Sell2025!', gen_salt('bf', 12))),
    (2, 'Николаев Виктор Олегович',  'seller',    43000, 0.10,
        'seller_spb_1', crypt('Sell2025!', gen_salt('bf', 12))),
    (3, 'Хасанов Дамир Ренатович',   'seller',    40000, 0.10,
        'seller_kzn_1', crypt('Sell2025!', gen_salt('bf', 12)));
```

### 5.6 Клиенты

```sql
INSERT INTO clients (full_name, phone, email, login, password_hash, discount_pct) VALUES
    ('Сидорова Мария Александровна', '+79001234567', 'sidorova@mail.ru',
        'client_001', crypt('pass123', gen_salt('bf', 12)), 0.05),
    ('Волков Дмитрий Сергеевич',     '+79009876543', 'volkov@gmail.com',
        'client_002', crypt('pass123', gen_salt('bf', 12)), 0.03),
    ('Новикова Юлия Андреевна',      '+79005553322', 'novikova@ya.ru',
        'client_003', crypt('pass123', gen_salt('bf', 12)), 0.10),
    ('Морозов Игорь Валентинович',   '+79004441100', NULL,
        'client_004', crypt('pass123', gen_salt('bf', 12)), 0.00),
    ('Лебедева Ксения Игоревна',     '+79007772255', 'lebedeva@mail.ru',
        'client_005', crypt('pass123', gen_salt('bf', 12)), 0.07);
```

### 5.7 Пополнить микросклады

```sql
SELECT replenish_micro_warehouse(1, 1, 30);
SELECT replenish_micro_warehouse(2, 2, 30);
SELECT replenish_micro_warehouse(3, 3, 30);
```

### 5.8 Создать тестовые заказы

```sql
SELECT create_order(1, 1, 4,
    '[{"product_id":1,"qty":1},{"product_id":8,"qty":2},{"product_id":10,"qty":1}]'::jsonb);

SELECT create_order(1, 2, 5,
    '[{"product_id":5,"qty":1}]'::jsonb);

SELECT create_order(2, 3, 6,
    '[{"product_id":2,"qty":1},{"product_id":6,"qty":1}]'::jsonb);

SELECT create_order(1, 4, 4,
    '[{"product_id":13,"qty":1},{"product_id":3,"qty":1}]'::jsonb);

SELECT create_order(3, 5, 7,
    '[{"product_id":7,"qty":1},{"product_id":9,"qty":2}]'::jsonb);
```

### 5.9 Изменить зарплату (создаст запись в salary_log)

```sql
SELECT update_salary(4, 48000, 1);
SELECT update_salary(5, 44000, 1);
```

---

## 6. Тестирование

### 6.1 Проверить структуру таблиц

```sql
SELECT table_name, column_name, data_type, is_nullable
FROM information_schema.columns
WHERE table_schema = 'public'
ORDER BY table_name, ordinal_position;
```

### 6.2 Проверить все индексы

```sql
SELECT indexname, tablename, indexdef
FROM pg_indexes
WHERE schemaname = 'public'
ORDER BY tablename, indexname;
```

### 6.3 Проверить все функции

```sql
SELECT routine_name, routine_type
FROM information_schema.routines
WHERE routine_schema = 'public'
ORDER BY routine_name;
```

### 6.4 Проверить заполнение таблиц

```sql
SELECT
    'shops'              AS tbl, COUNT(*) FROM shops              UNION ALL
SELECT 'warehouses',              COUNT(*) FROM warehouses         UNION ALL
SELECT 'products',                COUNT(*) FROM products           UNION ALL
SELECT 'warehouse_stock',         COUNT(*) FROM warehouse_stock    UNION ALL
SELECT 'employees',               COUNT(*) FROM employees          UNION ALL
SELECT 'clients',                 COUNT(*) FROM clients            UNION ALL
SELECT 'orders',                  COUNT(*) FROM orders             UNION ALL
SELECT 'order_items',             COUNT(*) FROM order_items        UNION ALL
SELECT 'salary_log',              COUNT(*) FROM salary_log         UNION ALL
SELECT 'stock_transfer_log',      COUNT(*) FROM stock_transfer_log;
```

### 6.5 Тест аутентификации — все роли

```sql
SELECT * FROM authenticate_user('director_msk', 'Dir2025!');
SELECT * FROM authenticate_user('seller_msk_1', 'Sell2025!');
SELECT * FROM authenticate_user('client_001',   'pass123');
SELECT * FROM authenticate_user('director_msk', 'wrong_password');
```

### 6.6 Тест расчёта цены

```sql
SELECT
    p.name,
    p.cost_price                                        AS себестоимость,
    calc_sale_price(p.cost_price, 0.10, 0.10, 0.00)   AS цена_без_скидки,
    calc_sale_price(p.cost_price, 0.10, 0.10, 0.05)   AS цена_скидка_5пр,
    calc_sale_price(p.cost_price, 0.10, 0.10, 0.10)   AS цена_скидка_10пр
FROM products
ORDER BY category, name;
```

### 6.7 Тест просмотра склада

```sql
SELECT warehouse_type, shop_name, product_name, category, quantity, cost_price
FROM v_stock_overview
ORDER BY warehouse_type, shop_name, category, product_name;
```

### 6.8 Тест создания заказа — нормальный

```sql
SELECT create_order(
    1,
    1,
    4,
    '[{"product_id": 4, "qty": 2}]'::jsonb
);
```

### 6.9 Тест создания заказа — недостаточно товара (ожидается ошибка)

```sql
SELECT create_order(
    1,
    1,
    4,
    '[{"product_id": 1, "qty": 9999}]'::jsonb
);
```

### 6.10 Тест отмены заказа директором

```sql
SELECT cancel_order(1, 1);

SELECT id, status, cancelled_by, cancelled_at
FROM orders
WHERE id = 1;
```

### 6.11 Тест отмены заказа — не директор (ожидается ошибка)

```sql
SELECT cancel_order(2, 4);
```

### 6.12 Тест пополнения микросклада

```sql
SELECT replenish_micro_warehouse(1, 1, 50);

SELECT product_name, shop_name, warehouse_type, quantity
FROM v_stock_overview
WHERE shop_name = 'PC Shop Москва'
ORDER BY product_name;
```

### 6.13 Тест нанять директора — второй в магазин (ожидается ошибка)

```sql
SELECT hire_employee(1, 'Второй директор', 'director', 100000, 0.10, 'dir2_msk', '123456');
```

### 6.14 Тест увольнения — чужой директор (ожидается ошибка)

```sql
SELECT fire_employee(6, 1);
```

### 6.15 Просмотр заказов с деталями

```sql
SELECT
    o.id,
    to_char(o.created_at, 'DD.MM.YYYY HH24:MI') AS дата,
    s.name                                        AS магазин,
    c.full_name                                   AS клиент,
    e.full_name                                   AS продавец,
    o.total_amount                                AS сумма,
    (o.discount_pct * 100) || '%'                 AS скидка,
    o.status
FROM orders o
JOIN shops     s ON s.id = o.shop_id
JOIN clients   c ON c.id = o.client_id
JOIN employees e ON e.id = o.seller_id
ORDER BY o.created_at DESC;
```

### 6.16 Состав заказа

```sql
SELECT
    o.id                                              AS заказ,
    p.name                                            AS товар,
    oi.product_category                               AS категория,
    oi.quantity                                       AS кол_во,
    oi.cost_price                                     AS себестоимость,
    oi.sale_price                                     AS цена_продажи,
    oi.sale_price * oi.quantity                       AS итого,
    (oi.sale_price - oi.cost_price) * oi.quantity     AS прибыль
FROM order_items oi
JOIN products p ON p.id = oi.product_id
JOIN orders   o ON o.id = oi.order_id
ORDER BY o.id, p.name;
```

### 6.17 Зарплатная ведомость

```sql
SELECT
    employee_id,
    full_name,
    base_salary,
    commission,
    total_earnings,
    to_char(month, 'MM.YYYY') AS месяц
FROM v_seller_earnings_monthly
ORDER BY month DESC, total_earnings DESC;
```

### 6.18 Топ товаров

```sql
SELECT product_name, product_category, total_sold, total_revenue, total_profit
FROM v_top_products
ORDER BY total_sold DESC;
```

### 6.19 Тест смены пароля

```sql
SELECT change_password(1, 'client', 'pass123', 'newpass456');

SELECT change_password(1, 'client', 'pass123', 'newpass456');
```

### 6.20 Тест полнотекстового поиска

```sql
SELECT name, category, cost_price,
       ts_rank(search_vec, to_tsquery('russian', 'процессор')) AS rank
FROM products
WHERE search_vec @@ to_tsquery('russian', 'процессор')
ORDER BY rank DESC;
```

### 6.21 Тест нечёткого поиска (триграммы)

```sql
SELECT name, category, similarity(lower(name), 'интел') AS sml
FROM products
WHERE lower(name) % 'интел'
ORDER BY sml DESC;
```

### 6.22 История изменений зарплат

```sql
SELECT
    sl.changed_at,
    e.full_name  AS сотрудник,
    sl.old_salary,
    sl.new_salary,
    sl.new_salary - sl.old_salary AS изменение,
    d.full_name  AS изменил
FROM salary_log sl
JOIN employees e ON e.id = sl.employee_id
JOIN employees d ON d.id = sl.changed_by
ORDER BY sl.changed_at DESC;
```

### 6.23 История переводов между складами

```sql
SELECT
    stl.transferred_at,
    p.name      AS товар,
    stl.quantity,
    fw.name     AS откуда,
    tw.name     AS куда,
    e.full_name AS инициатор
FROM stock_transfer_log stl
JOIN products   p  ON p.id  = stl.product_id
JOIN warehouses fw ON fw.id = stl.from_warehouse
JOIN warehouses tw ON tw.id = stl.to_warehouse
JOIN employees  e  ON e.id  = stl.initiated_by
ORDER BY stl.transferred_at DESC;
```

### 6.24 Выручка по месяцам (Москва)

```sql
SELECT
    to_char(DATE_TRUNC('month', o.created_at), 'MM.YYYY') AS месяц,
    COUNT(*)                                                AS заказов,
    SUM(o.total_amount)                                     AS выручка,
    SUM(o.total_amount) - SUM(sub.cost)                     AS валовая_прибыль
FROM orders o
JOIN LATERAL (
    SELECT SUM(oi.cost_price * oi.quantity) AS cost
    FROM order_items oi
    WHERE oi.order_id = o.id
) sub ON TRUE
WHERE o.shop_id = 1
  AND o.status  = 'completed'
GROUP BY DATE_TRUNC('month', o.created_at)
ORDER BY 1 DESC;
```

### 6.25 Продажи по категориям без JOIN к orders (денормализация)

```sql
SELECT
    product_category                 AS категория,
    SUM(quantity)                    AS продано_штук,
    SUM(sale_price * quantity)       AS выручка,
    SUM((sale_price - cost_price) * quantity) AS прибыль
FROM order_items
WHERE shop_id = 1
GROUP BY product_category
ORDER BY выручка DESC;
```

### 6.26 Обзор всех магазинов

```sql
SELECT
    s.id,
    s.name,
    CASE WHEN s.is_open THEN 'Открыт' ELSE 'Закрыт' END AS статус,
    (s.director_margin * 100) || '%'                      AS маржа_директора,
    COALESCE(d.full_name, 'Не назначен')                  AS директор,
    COUNT(DISTINCT e.id) FILTER (WHERE e.role = 'seller' AND e.is_active) AS продавцов,
    COALESCE(SUM(o.total_amount) FILTER (WHERE o.status = 'completed'), 0) AS выручка_всего
FROM shops s
LEFT JOIN employees d ON d.shop_id = s.id AND d.role = 'director' AND d.is_active
LEFT JOIN employees e ON e.shop_id = s.id
LEFT JOIN orders    o ON o.shop_id = s.id
GROUP BY s.id, s.name, s.is_open, s.director_margin, d.full_name
ORDER BY s.id;
```

---

## 7. Очистка / сброс данных (для повторного тестирования)

```sql
TRUNCATE TABLE stock_transfer_log RESTART IDENTITY CASCADE;
TRUNCATE TABLE salary_log         RESTART IDENTITY CASCADE;
TRUNCATE TABLE order_items        RESTART IDENTITY CASCADE;
TRUNCATE TABLE orders             RESTART IDENTITY CASCADE;
TRUNCATE TABLE warehouse_stock    RESTART IDENTITY CASCADE;
TRUNCATE TABLE employees          RESTART IDENTITY CASCADE;
TRUNCATE TABLE clients            RESTART IDENTITY CASCADE;
TRUNCATE TABLE warehouses         RESTART IDENTITY CASCADE;
TRUNCATE TABLE products           RESTART IDENTITY CASCADE;
TRUNCATE TABLE shops              RESTART IDENTITY CASCADE;
```

> Полный сброс схемы (удалить всё и начать заново):
> ```sql
> DROP SCHEMA public CASCADE;
> CREATE SCHEMA public;
> GRANT ALL ON SCHEMA public TO postgres;
> GRANT USAGE ON SCHEMA public TO crm_app;
> ```

---

## Тестовые логины

| Роль | Логин | Пароль |
|---|---|---|
| Директор (Москва) | `director_msk` | `Dir2025!` |
| Директор (СПб) | `director_spb` | `Dir2025!` |
| Директор (Казань) | `director_kzn` | `Dir2025!` |
| Продавец (Москва) 1 | `seller_msk_1` | `Sell2025!` |
| Продавец (Москва) 2 | `seller_msk_2` | `Sell2025!` |
| Продавец (СПб) | `seller_spb_1` | `Sell2025!` |
| Продавец (Казань) | `seller_kzn_1` | `Sell2025!` |
| Клиент (5% скидка) | `client_001` | `pass123` |
| Клиент (10% скидка) | `client_003` | `pass123` |
