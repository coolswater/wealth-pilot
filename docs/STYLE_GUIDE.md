# WealthPilot 页面样式规范

## 1. 概述

本文档定义了 WealthPilot 项目中所有页面的统一样式规范，确保视觉一致性和代码可维护性。

## 2. 颜色系统

### 2.1 使用 Tokens 命名空间

所有颜色必须使用 `Tokens::Colors` 命名空间中定义的颜色常量，禁止硬编码颜色值。

```cpp
// ✅ 正确
QString color = Tokens::Colors::Primary;
QString bgColor = Tokens::Colors::BgBase;

// ❌ 错误
QString color = "#3B82F6";
QString bgColor = "#0F1419";
```

### 2.2 核心颜色定义

| 用途 | 颜色常量 | 说明 |
|------|---------|------|
| 主色调 | `Colors::Primary` | 主要按钮、链接、高亮 |
| 主色调悬停 | `Colors::PrimaryHover` | 主色调悬停状态 |
| 主色调深色 | `Colors::PrimaryDark` | 主色调按下状态 |
| 主色调浅色 | `Colors::PrimaryLight` | 主色调浅色背景 |
| 成功色 | `Colors::Success` | 上涨、成功状态（绿色） |
| 危险色 | `Colors::Danger` | 下跌、错误状态（红色） |
| 警告色 | `Colors::Warning` | 警告状态（黄色） |
| 背景基础 | `Colors::BgBase` | 页面背景色 |
| 背景提升 | `Colors::BgElevated` | 卡片、面板背景色 |
| 背景表面 | `Colors::BgSurface` | 表面元素背景色 |
| 背景悬停 | `Colors::BgHover` | 悬停状态背景色 |
| 文字主色 | `Colors::TextPrimary` | 主要文字颜色 |
| 文字次色 | `Colors::TextSecondary` | 次要文字颜色 |
| 文字三级 | `Colors::TextTertiary` | 三级文字颜色 |
| 文字禁用 | `Colors::TextDisabled` | 禁用状态文字颜色 |
| 边框色 | `Colors::Border` | 边框颜色 |

### 2.3 涨跌颜色规范

中国市场：**红涨绿跌**

```cpp
// 上涨 - 红色
QString upColor = Tokens::Colors::Danger;  // #EF4444

// 下跌 - 绿色
QString downColor = Tokens::Colors::Success;  // #10B981

// 平盘 - 灰色
QString flatColor = Tokens::Colors::TextSecondary;
```

## 3. PageStyles 工具类

### 3.1 使用 PageStyles 统一样式

所有页面应使用 `PageStyles` 工具类提供的样式方法，避免重复定义样式字符串。

```cpp
#include "ui/components/PageStyles.h"

// 页面标题
titleLabel->setStyleSheet(PageStyles::titleText());

// 分组框
groupBox->setStyleSheet(PageStyles::groupBox());

// 输入框
lineEdit->setStyleSheet(PageStyles::inputField());

// 下拉框
comboBox->setStyleSheet(PageStyles::comboBox());

// 主按钮
primaryBtn->setStyleSheet(PageStyles::primaryButton());

// 次按钮
secondaryBtn->setStyleSheet(PageStyles::secondaryButton());

// 复选框
checkBox->setStyleSheet(PageStyles::checkBox());

// 表格
tableWidget->setStyleSheet(PageStyles::table());
```

### 3.2 PageStyles 方法列表

| 方法 | 用途 | 返回值 |
|------|------|--------|
| `titleText()` | 页面标题样式 | QString |
| `subtitleText()` | 副标题样式 | QString |
| `valueText(color)` | 数值文字样式 | QString |
| `labelText()` | 标签文字样式 | QString |
| `primaryButton()` | 主按钮样式 | QString |
| `secondaryButton()` | 次按钮样式 | QString |
| `inputField()` | 输入框样式 | QString |
| `comboBox()` | 下拉框样式 | QString |
| `checkBox()` | 复选框样式 | QString |
| `groupBox()` | 分组框样式 | QString |
| `table()` | 表格样式 | QString |
| `tabWidget()` | 标签页样式 | QString |
| `cardContainer()` | 卡片容器样式 | QString |
| `statCard(accentColor)` | 统计卡片样式 | QString |

## 4. 布局规范

### 4.1 页面布局结构

所有页面应遵循统一的布局结构：

```cpp
void setupUI()
{
    // 1. 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // 2. 页面标题栏
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel(QStringLiteral("页面标题"), this);
    titleLabel->setStyleSheet(PageStyles::titleText());
    headerLayout->addWidget(titleLabel);
    headerLayout->addStretch();
    // 添加操作按钮...
    mainLayout->addLayout(headerLayout);
    
    // 3. 内容区域（可选滚动区域）
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    
    QWidget* scrollContent = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setSpacing(16);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    
    // 4. 添加内容组件...
    
    contentLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);
}
```

### 4.2 间距规范

| 元素 | 间距值 | 说明 |
|------|--------|------|
| 页面外边距 | 24px (左右), 16px (上下) | `setContentsMargins(24, 16, 24, 16)` |
| 组件间距 | 16px | `setSpacing(16)` |
| 分组内间距 | 12px | `setSpacing(12)` |
| 卡片间距 | 12px | 卡片之间的间距 |
| 按钮间距 | 8px | 按钮之间的间距 |

### 4.3 尺寸规范

| 元素 | 尺寸 | 说明 |
|------|------|------|
| 主按钮高度 | 36px | 主要操作按钮 |
| 次按钮高度 | 32px | 次要操作按钮 |
| 输入框高度 | 32px | 文本输入框 |
| 下拉框高度 | 32px | 下拉选择框 |
| 分类按钮高度 | 32px | 分类切换按钮 |
| 卡片最小高度 | 140px | 内容卡片 |
| 圆角半径 | 6px | 按钮、卡片圆角 |
| 大圆角半径 | 8px | 分组框、面板圆角 |

## 5. 组件规范

### 5.1 QGroupBox 分组框

```cpp
QGroupBox* group = new QGroupBox(QStringLiteral("分组标题"), this);
group->setStyleSheet(PageStyles::groupBox());

QFormLayout* layout = new QFormLayout(group);
layout->setSpacing(12);
// 添加内容...
```

### 5.2 QPushButton 按钮

```cpp
// 主按钮
QPushButton* primaryBtn = new QPushButton(QStringLiteral("保存"), this);
primaryBtn->setStyleSheet(PageStyles::primaryButton());
primaryBtn->setFixedWidth(80);

// 次按钮
QPushButton* secondaryBtn = new QPushButton(QStringLiteral("取消"), this);
secondaryBtn->setStyleSheet(PageStyles::secondaryButton());
secondaryBtn->setFixedWidth(80);

// 图标按钮
QPushButton* iconBtn = new QPushButton(this);
iconBtn->setIcon(QIcon(":/icons/refresh.svg"));
iconBtn->setFixedSize(32, 32);
iconBtn->setCursor(Qt::PointingHandCursor);
```

### 5.3 QLineEdit 输入框

```cpp
QLineEdit* lineEdit = new QLineEdit(this);
lineEdit->setPlaceholderText(QStringLiteral("请输入..."));
lineEdit->setStyleSheet(PageStyles::inputField());
lineEdit->setMinimumWidth(200);
```

### 5.4 QComboBox 下拉框

```cpp
QComboBox* comboBox = new QComboBox(this);
comboBox->addItem(QStringLiteral("选项1"), "value1");
comboBox->addItem(QStringLiteral("选项2"), "value2");
comboBox->setStyleSheet(PageStyles::comboBox());
comboBox->setMinimumWidth(150);
```

### 5.5 QCheckBox 复选框

```cpp
QCheckBox* checkBox = new QCheckBox(QStringLiteral("选项文本"), this);
checkBox->setStyleSheet(PageStyles::checkBox());
```

### 5.6 QTableWidget 表格

```cpp
QTableWidget* table = new QTableWidget(this);
table->setColumnCount(5);
table->setHorizontalHeaderLabels({QStringLiteral("列1"), QStringLiteral("列2"), ...});
table->setStyleSheet(PageStyles::table());
table->horizontalHeader()->setStretchLastSection(true);
table->verticalHeader()->setVisible(false);
table->setSelectionBehavior(QAbstractItemView::SelectRows);
table->setSelectionMode(QAbstractItemView::SingleSelection);
table->setEditTriggers(QAbstractItemView::NoEditTriggers);
```

### 5.7 QLabel 标签

```cpp
// 标题标签
QLabel* titleLabel = new QLabel(QStringLiteral("标题"), this);
titleLabel->setStyleSheet(PageStyles::titleText());

// 普通标签
QLabel* label = new QLabel(QStringLiteral("标签文本"), this);
label->setStyleSheet(PageStyles::labelText());

// 数值标签
QLabel* valueLabel = new QLabel(QStringLiteral("123.45"), this);
valueLabel->setStyleSheet(PageStyles::valueText());

// 带颜色的数值标签
QLabel* profitLabel = new QLabel(QStringLiteral("+12.5%"), this);
profitLabel->setStyleSheet(PageStyles::valueText(Tokens::Colors::Success));
```

## 6. 编码规范

### 6.1 命名规范

```cpp
// 类名：大驼峰命名法
class DashboardPage : public BasePage { };

// 成员变量：m_ 前缀 + 小驼峰命名法
private:
    QLabel* m_titleLabel = nullptr;
    QPushButton* m_saveButton = nullptr;

// 局部变量：小驼峰命名法
QString pageTitle = QStringLiteral("仪表盘");

// 常量：全大写 + 下划线分隔
static constexpr int MAX_RETRY_COUNT = 3;
static constexpr int DEFAULT_TIMEOUT_MS = 5000;

// 枚举：大驼峰命名法
enum class ChartType {
    KLine,
    TimeShare
};

// 函数：小驼峰命名法
void loadDataFromDatabase();
void updateChartDisplay();
```

### 6.2 注释规范

```cpp
/**
 * @brief 加载数据从数据库
 * @details 从本地数据库加载K线数据，支持缓存机制
 * @return true 如果加载成功，false 如果加载失败
 */
bool loadFromDatabase();

/**
 * @brief 更新图表显示
 * @param data K线数据
 * @param period 周期类型
 */
void updateChartDisplay(const QVector<KLineData>& data, KLinePeriod period);

// 单行注释：说明复杂逻辑
// 检查缓存是否过期（5分钟）
if (cacheAge > 300) {
    loadFromNetwork();
}

// TODO 注释：标记待办事项
// TODO: 实现数据库存储功能
// FIXME: 修复内存泄漏问题
// NOTE: 注意线程安全问题
```

### 6.3 代码组织

```cpp
// 头文件组织顺序
// 1. 版权声明
// 2. 防止重复包含
// 3. 包含头文件
// 4. 前向声明
// 5. 常量定义
// 6. 类型定义
// 7. 类声明

// 源文件组织顺序
// 1. 版权声明
// 2. 包含头文件
// 3. 常量定义
// 4. 匿名命名空间
// 5. 辅助类/函数
// 6. 类实现
//    - 构造函数/析构函数
//    - 公共方法
//    - 保护方法
//    - 私有方法
//    - 槽函数
```

### 6.4 字符串处理

```cpp
// ✅ 正确：使用 QStringLiteral 宏
QString text = QStringLiteral("中文文本");
label->setText(QStringLiteral("标题"));

// ✅ 正确：使用 QString::arg() 格式化
QString msg = QStringLiteral("已加载 %1 条数据").arg(count);

// ❌ 错误：直接使用字符串字面量
QString text = "中文文本";  // 可能导致编码问题

// ❌ 错误：使用 + 连接字符串
QString msg = "已加载 " + QString::number(count) + " 条数据";  // 效率低
```

### 6.5 内存管理

```cpp
// ✅ 正确：使用 QObject 父子关系自动管理
QLabel* label = new QLabel(this);  // this 作为父对象
QPushButton* btn = new QPushButton(parentWidget);

// ✅ 正确：使用智能指针
std::unique_ptr<Impl> d;
QSharedPointer<Data> data = QSharedPointer<Data>::create();

// ✅ 正确：使用 Qt 信号槽自动删除
connect(reply, &QNetworkReply::finished, reply, &QObject::deleteLater);

// ❌ 错误：忘记设置父对象导致内存泄漏
QLabel* label = new QLabel();  // 没有父对象，需要手动删除
```

## 7. 性能优化

### 7.1 数据加载优化

```cpp
// ✅ 正确：使用三级缓存机制
void loadData() {
    // 1. 尝试从内存缓存加载
    if (loadFromCache()) {
        return;
    }
    
    // 2. 尝试从数据库加载
    if (loadFromDatabase()) {
        saveToCache();  // 保存到缓存
        return;
    }
    
    // 3. 从网络加载
    loadFromNetwork();
}

// ✅ 正确：异步加载大数据
QTimer::singleShot(0, this, [this]() {
    loadLargeData();
});

// ✅ 正确：分页加载
void loadPage(int page, int pageSize) {
    int offset = (page - 1) * pageSize;
    // 只加载当前页数据
}
```

### 7.2 UI 渲染优化

```cpp
// ✅ 正确：批量更新 UI
tableWidget->setUpdatesEnabled(false);
for (int i = 0; i < data.size(); ++i) {
    // 添加行...
}
tableWidget->setUpdatesEnabled(true);

// ✅ 正确：使用委托绘制
class ColorDelegate : public QStyledItemDelegate {
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        // 自定义绘制，避免创建多个控件
    }
};

// ✅ 正确：延迟加载图片
QPixmapCache::insert(key, pixmap);  // 使用 Qt 图片缓存
```

### 7.3 网络请求优化

```cpp
// ✅ 正确：批量请求
void requestBatchData(const QStringList& symbols) {
    QString url = QString("https://api.example.com/batch?symbols=%1")
        .arg(symbols.join(","));
    // 一次请求多个数据
}

// ✅ 正确：请求去重
QHash<QString, QNetworkReply*> pendingRequests;

void requestData(const QString& symbol) {
    if (pendingRequests.contains(symbol)) {
        return;  // 已有相同请求在进行中
    }
    // 发起新请求
}

// ✅ 正确：设置超时
QTimer::singleShot(10000, reply, [reply]() {
    if (reply->isRunning()) {
        reply->abort();
    }
});
```

### 7.4 定时器优化

```cpp
// ✅ 正确：合理设置刷新间隔
// 实时数据：5秒
dataSource->startAutoRefresh(5000);

// 非实时数据：60秒
dataSource->startAutoRefresh(60000);

// ✅ 正确：页面不可见时停止刷新
void hideEvent(QHideEvent* event) override {
    dataSource->stopAutoRefresh();
    BasePage::hideEvent(event);
}

void showEvent(QShowEvent* event) override {
    dataSource->startAutoRefresh(5000);
    BasePage::showEvent(event);
}
```

## 8. 错误处理

### 8.1 日志记录

```cpp
#include "utils/Logger.h"

// ✅ 正确：使用日志宏
LOG_INFO(QString("Page initialized: %1").arg(pageId()));
LOG_DEBUG(QString("Loading data from: %1").arg(url));
LOG_WARNING(QString("Cache miss for: %1").arg(key));
LOG_ERROR(QString("Failed to load data: %1").arg(errorString));

// ❌ 错误：使用 qDebug()
qDebug() << "Debug message";  // 不统一，难以控制
```

### 8.2 异常处理

```cpp
// ✅ 正确：检查返回值
bool success = saveToDatabase();
if (!success) {
    LOG_ERROR("Failed to save data to database");
    QMessageBox::warning(this, QStringLiteral("错误"), 
        QStringLiteral("保存数据失败，请重试。"));
    return;
}

// ✅ 正确：检查指针
if (!m_dataSource) {
    LOG_ERROR("Data source is null");
    return;
}

// ✅ 正确：检查数据有效性
if (data.isEmpty()) {
    LOG_WARNING("No data available");
    return;
}
```

## 9. 代码模板

### 9.1 标准页面模板

```cpp
/**
 * @file XxxPage.cpp
 * @brief XXX页面实现
 * @details 功能描述
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "XxxPage.h"
#include "ui/components/PageStyles.h"
#include "core/config/Tokens.h"
#include "utils/Logger.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

using namespace Tokens;

// ============================================================================
// 常量定义
// ============================================================================
namespace {
    const QString COLOR_BG = Colors::BgBase;
    const QString COLOR_CARD = Colors::BgElevated;
}

// ============================================================================
// XxxPage::Impl - 私有实现
// ============================================================================
struct XxxPage::Impl {
    // UI 组件
    QLabel* titleLabel = nullptr;
    QPushButton* refreshBtn = nullptr;
    
    // 数据
    QVector<DataItem> dataCache;
};

// ============================================================================
// XxxPage 实现
// ============================================================================
XxxPage::XxxPage(QWidget* parent)
    : BasePage(parent)
    , d(std::make_unique<Impl>())
{
    setupUI();
    loadData();
}

XxxPage::~XxxPage() = default;

QString XxxPage::pageId() const
{
    return QStringLiteral("XxxPage");
}

void XxxPage::initializePage()
{
    LOG_INFO("XxxPage initialized");
}

void XxxPage::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 16, 24, 16);
    
    // 标题栏
    setupHeader(mainLayout);
    
    // 内容区域
    setupContent(mainLayout);
}

void XxxPage::setupHeader(QVBoxLayout* mainLayout)
{
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    d->titleLabel = new QLabel(QStringLiteral("页面标题"), this);
    d->titleLabel->setStyleSheet(PageStyles::titleText());
    headerLayout->addWidget(d->titleLabel);
    
    headerLayout->addStretch();
    
    d->refreshBtn = new QPushButton(QStringLiteral("刷新"), this);
    d->refreshBtn->setStyleSheet(PageStyles::primaryButton());
    d->refreshBtn->setFixedWidth(80);
    connect(d->refreshBtn, &QPushButton::clicked, this, &XxxPage::onRefresh);
    headerLayout->addWidget(d->refreshBtn);
    
    mainLayout->addLayout(headerLayout);
}

void XxxPage::setupContent(QVBoxLayout* mainLayout)
{
    // 添加内容组件
}

void XxxPage::loadData()
{
    LOG_INFO("Loading data...");
    // 实现数据加载逻辑
}

void XxxPage::onRefresh()
{
    loadData();
    LOG_INFO("Data refreshed");
}
```

## 10. 检查清单

### 10.1 样式检查

- [ ] 所有颜色使用 `Tokens::Colors` 常量
- [ ] 使用 `PageStyles` 工具类方法
- [ ] 布局间距符合规范
- [ ] 组件尺寸符合规范
- [ ] 涨跌颜色正确（红涨绿跌）

### 10.2 代码质量检查

- [ ] 命名符合规范
- [ ] 注释完整清晰
- [ ] 无硬编码字符串
- [ ] 无内存泄漏
- [ ] 错误处理完善

### 10.3 性能检查

- [ ] 使用缓存机制
- [ ] 批量更新 UI
- [ ] 合理的刷新间隔
- [ ] 页面不可见时停止刷新
- [ ] 网络请求优化

---

**版本历史：**
- v1.0.0 (2026-05-06) - 初始版本
