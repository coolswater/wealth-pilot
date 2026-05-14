-- Migration: v002_add_alerts.sql
-- Description: Add alerts table for price alerts
-- Version: 2

-- 警告表
CREATE TABLE IF NOT EXISTS alerts
(
    id
    INTEGER
    PRIMARY
    KEY
    AUTOINCREMENT,
    user_id
    INTEGER,
    symbol
    TEXT
    NOT
    NULL,
    alert_type
    TEXT
    NOT
    NULL, -- 'price_above', 'price_below', 'change_percent'
    target_price
    REAL,
    condition
    TEXT,
    enabled
    INTEGER
    DEFAULT
    1,
    created_at
    TEXT
    DEFAULT
    CURRENT_TIMESTAMP,
    triggered_at
    TEXT,
    FOREIGN
    KEY
(
    user_id
) REFERENCES users
(
    id
) ON DELETE CASCADE
    );

-- 索引
CREATE INDEX IF NOT EXISTS idx_alerts_user ON alerts(user_id);
CREATE INDEX IF NOT EXISTS idx_alerts_symbol ON alerts(symbol);
CREATE INDEX IF NOT EXISTS idx_alerts_enabled ON alerts(enabled);