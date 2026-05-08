# 样式迁移进度跟踪

## 已完成迁移的文件

| 文件 | 状态 | 备注 |
|------|------|------|
| OrderDialog.cpp | 🔄 进行中 | 部分迁移 |

## 待迁移文件列表

### 高优先级（核心UI组件）
- [ ] src/ui/components/OrderDialog.cpp
- [ ] src/views/aboutus/AboutUSPage.cpp
- [ ] src/views/dashboard/DashboardPage.cpp
- [ ] src/views/settings/SettingsPage.cpp
- [ ] src/views/futures/FuturesQuotesPage.cpp

### 中优先级（交易相关）
- [ ] src/views/trading/TradeHistoryPage.cpp
- [ ] src/views/trading/ConditionOrderPage.cpp
- [ ] src/views/stock/StockKLinePage.cpp
- [ ] src/views/stock/StockTimeSharePage.cpp

### 低优先级（其他页面）
- [ ] src/views/market/MarketOverviewPage.cpp
- [ ] src/views/portfolio/PortfolioPage.cpp
- [ ] src/views/watchlist/WatchListPage.cpp

## 迁移统计

- 总计 PageStyles 调用: 107 处
- 已迁移: 5 处
- 剩余: 102 处

## 迁移规则

### 按钮样式
```cpp
// 旧代码
button->setStyleSheet(PageStyles::primaryButton());

// 新代码
StyleHelper::setPrimaryButton(button);
```

### 涨跌颜色
```cpp
// 旧代码
label->setStyleSheet(PageStyles::valueText(PageStyles::upColor()));

// 新代码
StyleHelper::setTrendUp(label);
```

### 输入框/下拉框
```cpp
// 旧代码
lineEdit->setStyleSheet(PageStyles::inputField());
comboBox->setStyleSheet(PageStyles::comboBox());

// 新代码
// 全局样式自动生效，无需设置
```

### 分组框
```cpp
// 旧代码
groupBox->setStyleSheet(PageStyles::groupBox());

// 新代码
// 全局样式自动生效，无需设置
```

## 注意事项

1. **保留动态颜色**：某些需要根据数值动态改变颜色的地方，使用 ThemeManager 获取颜色
2. **批量刷新**：多个控件同时更新时，使用 StyleHelper::refreshStyles() 批量刷新
3. **测试验证**：每个文件迁移后需要测试视觉效果

---

**更新时间**: 2026-05-08
