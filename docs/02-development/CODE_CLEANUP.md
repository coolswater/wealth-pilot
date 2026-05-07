# 代码冗余分析报告

## 一、发现的冗余代码

### 1.1 重复的样式设置

**问题**: 多个页面重复设置相同的样式，未使用 PageStyles 类

**位置**:

- `AboutUSPage.cpp:76` - font-size: 14px
- `BacktestPage.cpp:338` - font-size: 14px
- `PortfolioPage.cpp:435` - font-size: 14px
- `StockKLinePage.cpp:1053` - font-size: 14px

**建议**: 统一使用 `PageStyles::labelText()` 或 `PageStyles::subtitleText()`

### 1.2 硬编码颜色

**问题**: 部分代码仍使用硬编码颜色值

**位置**:

- `BacktestPage.cpp:338` - #ffffff
- `BacktestPage.cpp:489` - #00D4AA
- `BacktestPage.cpp:491` - #FF3366
- `StockKLinePage.cpp:1053` - #ff4d4f

**建议**: 使用 `Tokens::Colors` 命名空间

### 1.3 重复的表格初始化

**问题**: 多个页面重复相似的表格初始化代码

**建议**: 创建 `TableUtils::initStandardTable()` 工具函数

### 1.4 未使用的空函数

**问题**: 部分回调函数为空实现

**位置**:

- `CryptoDataSource.cpp:45` - 空回调
- `ForexDataSource.cpp:43` - 空回调
- `FundDataSource.cpp:40` - 空回调

**建议**: 可以保留，用于占位回调

## 二、建议的优化

### 2.1 创建样式工具函数

```cpp
// 建议添加到 PageStyles.h
static QString standardLabel() {
    return QString("font-size: 14px; color: %1;")
        .arg(Tokens::Colors::TextSecondary);
}
```

### 2.2 创建表格工具类

```cpp
// 建议创建 TableUtils.h
class TableUtils {
public:
    static void initStandardTable(QTableWidget* table);
    static void setHeaderLabels(QTableWidget* table, const QStringList& labels);
};
```

### 2.3 统一颜色使用

将所有硬编码颜色替换为 Tokens::Colors

## 三、清理优先级

| 优先级 | 任务            | 影响     |
|-----|---------------|--------|
| 高   | 替换硬编码颜色       | 样式一致性  |
| 中   | 使用 PageStyles | 代码复用   |
| 低   | 创建工具类         | 减少重复代码 |

## 四、已清理项目

- [x] Tokens.h 添加浅色/护眼主题颜色
- [x] PageStyles.h 添加主题样式函数
- [x] ForexPage.cpp 使用 Tokens::Colors
- [ ] 其他页面硬编码颜色替换
- [ ] 创建表格工具类
