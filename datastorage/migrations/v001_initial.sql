-- Migration: v001_initial.sql
-- Description: Create initial database tables
-- Version: 1

-- 用户表
CREATE TABLE IF NOT EXISTS users
(
    id
    INTEGER
    PRIMARY
    KEY
    AUTOINCREMENT,
    username
    TEXT
    UNIQUE
    NOT
    NULL,
    password_hash
    TEXT
    NOT
    NULL,
    email
    TEXT,
    created_at
    TEXT
    DEFAULT
    CURRENT_TIMESTAMP,
    updated_at
    TEXT
    DEFAULT
    CURRENT_TIMESTAMP
);

-- 自选股表
CREATE TABLE IF NOT EXISTS watchlist
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
    name
    TEXT,
    added_at
    TEXT
    DEFAULT
    CURRENT_TIMESTAMP,
    sort_order
    INTEGER
    DEFAULT
    0,
    FOREIGN
    KEY
(
    user_id
) REFERENCES users
(
    id
) ON DELETE CASCADE
    );

-- 设置表
CREATE TABLE IF NOT EXISTS settings
(
    key
    TEXT
    PRIMARY
    KEY,
    value
    TEXT,
    updated_at
    TEXT
    DEFAULT
    CURRENT_TIMESTAMP
);

-- 索引
CREATE INDEX IF NOT EXISTS idx_watchlist_user ON watchlist(user_id);
CREATE INDEX IF NOT EXISTS idx_watchlist_symbol ON watchlist(symbol);