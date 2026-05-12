# WealthPilot 按钮设计规范

## 概述

本文档定义 WealthPilot 项目中所有按钮的统一设计规范，解决以下问题：

- 按钮样式不统一
- 文字显示不全
- 尺寸不规范
- 语义不清晰

## 按钮分类体系

### 一、按语义分类

#### 1. 主要操作按钮 (Primary)

**用途**：页面上最重要的操作，引导用户完成核心任务

| 场景   | 示例             |
|------|----------------|
| 保存操作 | 保存、保存设置、保存修改   |
| 提交操作 | 提交订单、提交申请、确认提交 |
| 确认操作 | 确认、确定、完成       |
| 运行操作 | 运行回测、执行优化、开始分析 |

**样式特点**：

- 品牌主色背景（蓝色）
- 白色文字
- 字体加粗
- 最小宽度 80px

```cpp
// 使用示例
auto* saveBtn = new QPushButton("保存");
ButtonStyles::setPrimary(saveBtn);
```

---

#### 2. 次要操作按钮 (Secondary)

**用途**：次要操作，不引导用户执行

| 场景   | 示例      |
|------|---------|
| 取消操作 | 取消、放弃修改 |
| 关闭操作 | 关闭、返回   |
| 辅助操作 | 稍后提醒、跳过 |

**样式特点**：

- 透明背景
- 灰色边框
- 灰色文字
- 悬停时边框变蓝

```cpp
auto* cancelBtn = new QPushButton("取消");
ButtonStyles::setSecondary(cancelBtn);
```

---

#### 3. 状态按钮

##### 3.1 成功按钮 (Success)

**用途**：正向、确认类操作

| 场景   | 示例    |
|------|-------|
| 买入操作 | 买入、开仓 |
| 启用操作 | 启用、激活 |
| 订阅操作 | 订阅、关注 |
| 添加操作 | 添加自选  |

```cpp
auto* buyBtn = new QPushButton("买入");
ButtonStyles::setSuccess(buyBtn);
```

##### 3.2 危险按钮 (Danger)

**用途**：危险、删除类操作

| 场景   | 示例      |
|------|---------|
| 卖出操作 | 卖出、平仓   |
| 删除操作 | 删除、移除   |
| 清空操作 | 清空、清空历史 |
| 停止操作 | 停止、终止   |

```cpp
auto* deleteBtn = new QPushButton("删除");
ButtonStyles::setDanger(deleteBtn);
```

##### 3.3 警告按钮 (Warning)

**用途**：警告类操作

| 场景   | 示例      |
|------|---------|
| 重置操作 | 重置、恢复默认 |
| 清理操作 | 清除缓存    |
| 风险操作 | 强制卖出    |

```cpp
auto* resetBtn = new QPushButton("重置");
ButtonStyles::setWarning(resetBtn);
```

##### 3.4 信息按钮 (Info)

**用途**：信息查看类操作

| 场景   | 示例      |
|------|---------|
| 查看操作 | 详情、查看详情 |
| 帮助操作 | 帮助、说明   |
| 分析操作 | 分析报告    |

```cpp
auto* detailBtn = new QPushButton("详情");
ButtonStyles::setInfo(detailBtn);
```

---

#### 4. 功能按钮

| 类型      | 用途   | 示例           |
|---------|------|--------------|
| Refresh | 刷新数据 | 刷新行情、刷新列表    |
| Add     | 添加项目 | 添加预警、添加股票    |
| Edit    | 编辑项目 | 修改、编辑        |
| Delete  | 删除项目 | 删除选中         |
| Export  | 导出数据 | 导出报告、导出Excel |
| Import  | 导入数据 | 导入配置         |
| Search  | 搜索操作 | 搜索、查询        |

```cpp
auto* refreshBtn = new QPushButton("刷新");
ButtonStyles::setRefresh(refreshBtn);

auto* addBtn = new QPushButton("添加预警");
ButtonStyles::setAdd(addBtn);
```

---

#### 5. 工具按钮

| 类型   | 用途    | 样式特点            |
|------|-------|-----------------|
| Icon | 纯图标按钮 | 无背景、无边框、24x24px |
| Text | 纯文本按钮 | 无背景、无边框         |
| Link | 链接样式  | 蓝色下划线文字         |

```cpp
auto* iconBtn = new QPushButton();
iconBtn->setIcon(QIcon(":/icons/refresh.svg"));
ButtonStyles::setIcon(iconBtn);

auto* linkBtn = new QPushButton("了解更多");
ButtonStyles::setLink(linkBtn);
```

---

#### 6. 对话框按钮

| 类型           | 用途   | 样式               |
|--------------|------|------------------|
| DialogAccept | 确认操作 | 主按钮样式，最小宽度 80px  |
| DialogReject | 取消操作 | 次要按钮样式，最小宽度 80px |
| DialogApply  | 应用操作 | 成功按钮样式           |
| DialogHelp   | 帮助操作 | 信息按钮样式           |

```cpp
auto* okBtn = new QPushButton("确定");
ButtonStyles::applyStyle(okBtn, ButtonType::DialogAccept);

auto* cancelBtn = new QPushButton("取消");
ButtonStyles::applyStyle(cancelBtn, ButtonType::DialogReject);
```

---

### 二、按尺寸分类

| 尺寸         | 高度   | 字体   | 内边距  | 适用场景       |
|------------|------|------|------|------------|
| Small      | 24px | 11px | 6px  | 表格内、紧凑布局   |
| Medium     | 32px | 13px | 10px | 默认尺寸，大多数场景 |
| Large      | 40px | 14px | 14px | 重要操作、对话框   |
| ExtraLarge | 48px | 16px | 18px | 引导页、空状态    |

```cpp
// 设置按钮尺寸
ButtonStyles::setLarge(saveBtn);
ButtonStyles::setSmall(tableBtn);
```

---

## 使用规范

### 1. 按钮文字规范

#### 文字长度

- **推荐**：2-4 个汉字
- **最大**：6 个汉字（超过需换行或省略）
- **最小宽度**：60px（防止文字截断）

#### 文字内容

- 使用动词开头：保存、删除、刷新
- 避免模糊：用"删除"而非"操作"
- 保持一致：统一用"保存"或"确定"

### 2. 按钮布局规范

#### 水平排列

```
[主要操作] [次要操作] [危险操作]
[保存]     [取消]     [删除]
```

#### 对话框按钮

```
                    [取消] [确定]
```

- 确定按钮在右侧
- 取消按钮在左侧

#### 工具栏按钮

```
[添加] [编辑] [删除] | [刷新] [导出]
```

- 相关操作分组
- 用分隔线区分不同组

### 3. 按钮状态规范

| 状态 | 视觉表现      |
|----|-----------|
| 正常 | 默认样式      |
| 悬停 | 背景加深，边框高亮 |
| 按下 | 背景更深      |
| 禁用 | 半透明，灰色文字  |
| 焦点 | 显示焦点框     |

### 4. 按钮禁用规范

以下情况应禁用按钮：

- 数据未加载完成
- 表单验证失败
- 无权限执行操作
- 操作进行中

```cpp
// 禁用按钮
button->setEnabled(false);

// 禁用时的提示
button->setToolTip("请先选择要删除的项目");
```

---

## 页面按钮分类清单

### DashboardPage（仪表盘）

| 按钮     | 类型        | 尺寸     | 说明    |
|--------|-----------|--------|-------|
| 关闭（弹窗） | Secondary | Medium | 关闭对话框 |

### StockQuotesPage（股票行情）

| 按钮 | 类型      | 尺寸     | 说明     |
|----|---------|--------|--------|
| 刷新 | Refresh | Medium | 刷新行情数据 |

### AlertCenterPage（预警中心）

| 按钮    | 类型      | 尺寸     | 说明     |
|-------|---------|--------|--------|
| 添加预警  | Add     | Medium | 添加新预警  |
| 删除    | Delete  | Medium | 删除选中预警 |
| 启用/禁用 | Toggle  | Medium | 切换预警状态 |
| 刷新    | Refresh | Medium | 刷新预警列表 |
| 清空历史  | Danger  | Medium | 清空历史记录 |

### BacktestPage（回测）

| 按钮   | 类型      | 尺寸     | 说明     |
|------|---------|--------|--------|
| 运行回测 | Primary | Large  | 执行回测   |
| 停止   | Danger  | Medium | 停止回测   |
| 导出报告 | Export  | Medium | 导出回测报告 |

### TradingPanel（交易面板）

| 按钮   | 类型        | 尺寸     | 说明     |
|------|-----------|--------|--------|
| 提交订单 | Primary   | Large  | 提交交易订单 |
| 取消   | Secondary | Medium | 取消订单   |
| 重置   | Warning   | Medium | 重置表单   |
| 计算   | Info      | Medium | 计算费用   |

### SettingsPage（设置）

| 按钮   | 类型      | 尺寸     | 说明     |
|------|---------|--------|--------|
| 保存   | Primary | Medium | 保存设置   |
| 重置   | Warning | Medium | 重置为默认  |
| 清除缓存 | Warning | Medium | 清除缓存   |
| 导出数据 | Export  | Medium | 导出用户数据 |

### CTPBrokerDialog（CTP配置）

| 按钮               | 类型        | 尺寸     | 说明    |
|------------------|-----------|--------|-------|
| Add              | Add       | Small  | 添加经纪商 |
| Edit             | Edit      | Small  | 编辑经纪商 |
| Delete           | Delete    | Small  | 删除经纪商 |
| Reset Defaults   | Warning   | Small  | 重置默认  |
| Save Credentials | Primary   | Medium | 保存凭证  |
| Test Connection  | Info      | Medium | 测试连接  |
| Switch Broker    | Primary   | Medium | 切换经纪商 |
| Close            | Secondary | Medium | 关闭对话框 |

---

## 迁移指南

### 从旧样式迁移到新样式

#### 步骤 1：识别按钮类型

根据按钮的功能确定其类型（Primary/Secondary/Danger等）

#### 步骤 2：修改代码

**旧代码：**

```cpp
auto* saveBtn = new QPushButton("保存");
saveBtn->setStyleSheet("background-color: #58A6FF; color: white; border-radius: 6px;");
```

**新代码：**

```cpp
auto* saveBtn = new QPushButton("保存");
ButtonStyles::setPrimary(saveBtn);
```

#### 步骤 3：设置尺寸（可选）

```cpp
ButtonStyles::setLarge(saveBtn);
```

#### 步骤 4：设置最小宽度（防止截断）

```cpp
ButtonStyles::setMinWidth(saveBtn, 4);  // 至少4个字符宽度
```

---

## 最佳实践

### 1. 保持一致性

- 同一页面相同功能的按钮使用相同样式
- 遵循设计规范，不要随意创建新样式

### 2. 语义清晰

- 按钮文字明确表达操作结果
- 危险操作使用红色警示

### 3. 适当留白

- 按钮之间保持 8px 间距
- 按钮组与周围元素保持 16px 间距

### 4. 响应式设计

- 按钮最小宽度确保文字不截断
- 长文本考虑换行或省略

### 5. 无障碍设计

- 禁用状态提供提示信息
- 支持键盘导航
- 焦点状态明显

---

## 相关文件

- `src/ui/styles/ButtonStyles.h` - 按钮样式管理类
- `src/ui/styles/ButtonStyles.cpp` - 实现文件
- `resources/style/buttons.qss` - QSS 样式定义
- `src/ui/components/StyleHelper.h` - 样式辅助工具
