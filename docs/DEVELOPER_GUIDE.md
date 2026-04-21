# WealthPilot 开发者指南

## 目录

1. [开发环境搭建](#1-开发环境搭建)
2. [项目架构](#2-项目架构)
3. [核心模块](#3-核心模块)
4. [开发规范](#4-开发规范)
5. [扩展开发](#5-扩展开发)
6. [调试技巧](#6-调试技巧)

---

## 1. 开发环境搭建

### 1.1 必需软件

| 软件 | 版本 | 用途 |
|------|------|------|
| Qt | 6.10.2 | GUI框架 |
| CMake | 3.16+ | 构建系统 |
| MinGW | 64-bit | 编译器 |
| Git | 最新版 | 版本控制 |
| Qt Creator | 最新版 | IDE（可选） |

### 1.2 环境配置

#### Windows 环境变量
```
QT_DIR=C:\Qt\6.10.2\mingw_64
CMAKE_PREFIX_PATH=%QT_DIR%
PATH=%QT_DIR%\bin;C:\Qt\Tools\mingw1630_64\bin;C:\Qt\Tools\CMake_64\bin;%PATH%
```

### 1.3 克隆与构建

```bash
# 克隆项目
git clone https://github.com/your-repo/wealth-pilot.git
cd wealth-pilot

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug ..

# 编译
cmake --build . --target WealthPilot -j4

# 运行
./WealthPilot.exe
```

---

## 2. 项目架构

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    Views Layer                          │
│  (DashboardPage, StockKLinePage, PortfolioPage, ...)   │
├─────────────────────────────────────────────────────────┤
│                    UI Components                        │
│  (KLineChart, MarketDepthWidget, CardWidget, ...)      │
├─────────────────────────────────────────────────────────┤
│                    Core Layer                           │
│  (Config, Cache, Database, Navigation, Task, Types)    │
├─────────────────────────────────────────────────────────┤
│                    Services Layer                       │
│  (CTP API, AI Service, Market Data, Trading)           │
└─────────────────────────────────────────────────────────┘
```

### 2.2 模块依赖关系

```
app
 └── views
      ├── ui/components
      ├── core
      │    ├── config
      │    ├── cache
      │    ├── database
      │    ├── navigation
      │    └── task
      ├── trading
      ├── ctp
      ├── ai
      └── utils
```

### 2.3 设计模式

| 模式 | 应用场景 | 示例 |
|------|----------|------|
| 单例模式 | 全局管理器 | ThemeManager, ConfigManager |
| PIMPL模式 | 隐藏实现细节 | KLineChart::Impl |
| 工厂模式 | 页面创建 | PageFactoryRegistry |
| 观察者模式 | 事件通知 | Qt信号槽 |
| 依赖注入 | 服务解耦 | ServiceLocator |

---

## 3. 核心模块

### 3.1 设计系统 (Tokens)

所有设计变量定义在 `src/core/config/Tokens.h`：

```cpp
// 颜色
Tokens::Colors::Primary      // #3B82F6
Tokens::Colors::Danger       // #EF4444 (涨)
Tokens::Colors::Success      // #10B981 (跌)

// 间距
Tokens::Spacing::MD          // 16px

// 圆角
Tokens::Radius::LG           // 12px

// 字体
Tokens::Font::Size::Body     // 14px
```

### 3.2 导航系统

```cpp
// 注册页面
PageFactoryRegistry::instance().registerPage(
    "dashboard",
    []() -> BasePage* { return new DashboardPage(); }
);

// 导航到页面
PageNavigator::instance().navigateTo("dashboard");
```

### 3.3 配置管理

```cpp
// 读取配置
QString theme = ConfigManager::instance().get("theme/current", "dark");

// 写入配置
ConfigManager::instance().set("theme/current", "light");
```

### 3.4 数据库

```cpp
// 执行查询
QSqlQuery query = DatabaseManager::instance().executeQuery(
    "SELECT * FROM stocks WHERE code = ?", {symbol}
);
```

---

## 4. 开发规范

### 4.1 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 类名 | 大驼峰 | `DashboardPage` |
| 函数名 | 小驼峰 | `updateData()` |
| 变量名 | 小驼峰 | `totalCount` |
| 成员变量 | m_前缀 | `m_currentTheme` |
| 常量 | 全大写 | `MAX_COUNT` |
| 命名空间 | 大驼峰 | `Tokens` |

### 4.2 文件组织

```cpp
// 头文件结构
/**
 * @file ClassName.h
 * @brief 简要描述
 * @details 详细描述
 * @author Author Name
 * @version 1.0.0
 */

#ifndef CLASSNAME_H
#define CLASSNAME_H

// Qt includes
#include <QWidget>

// STD includes
#include <memory>

// Project includes
#include "core/config/Tokens.h"

// Forward declarations
class QLabel;

// Class definition
class ClassName : public QWidget
{
    Q_OBJECT

public:
    explicit ClassName(QWidget* parent = nullptr);
    ~ClassName();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // CLASSNAME_H
```

### 4.3 注释规范

```cpp
/**
 * @brief 简要描述
 * @details 详细描述
 * @param param1 参数1说明
 * @param param2 参数2说明
 * @return 返回值说明
 * @throws 异常说明
 * @example
 * @code
 * // 使用示例
 * @endcode
 */
```

### 4.4 代码风格

- 使用 4 空格缩进
- 大括号独占一行
- 每行不超过 100 字符
- 使用 `auto` 简化类型声明
- 优先使用 `const` 和 `constexpr`

---

## 5. 扩展开发

### 5.1 添加新页面

1. **创建页面类**

```cpp
// src/views/mypage/MyPage.h
#ifndef MYPAGE_H
#define MYPAGE_H

#include <core/base/BasePage.h>

class MyPage : public BasePage
{
    Q_OBJECT

public:
    explicit MyPage(QWidget* parent = nullptr);
    ~MyPage();

    void initializePage() override;

private:
    void setupUI();
    void setupConnections();
};
#endif
```

2. **实现页面类**

```cpp
// src/views/mypage/MyPage.cpp
#include "MyPage.h"
#include "core/config/Tokens.h"

MyPage::MyPage(QWidget* parent) : BasePage(parent)
{
    setupUI();
    setupConnections();
}

void MyPage::initializePage()
{
    // 页面初始化逻辑
}

void MyPage::setupUI()
{
    setStyleSheet(QString("background-color: %1;")
        .arg(Tokens::Colors::BgBase));
}
```

3. **注册页面**

```cpp
// 在 main.cpp 或初始化代码中
PageFactoryRegistry::instance().registerPage(
    "mypage",
    []() -> BasePage* { return new MyPage(); }
);
```

4. **更新 CMakeLists.txt**

```cmake
set(VIEW_SOURCES
    # ... existing sources
    views/mypage/MyPage.cpp
)
```

### 5.2 添加新组件

1. 在 `src/ui/components/` 创建组件
2. 使用 PIMPL 模式隐藏实现
3. 使用 Tokens 定义样式
4. 添加到 `ComponentFactory`

### 5.3 添加新主题

1. 创建 `resources/style/theme_new.qss`
2. 在 `ThemeManager` 中注册主题
3. 更新 `Tokens.h` 添加主题颜色

---

## 6. 调试技巧

### 6.1 日志系统

```cpp
#include "utils/Logger.h"

LOG_DEBUG("调试信息");
LOG_INFO("普通信息");
LOG_WARNING("警告信息");
LOG_ERROR("错误信息");
```

### 6.2 常见问题排查

#### 编译错误
- 检查 Qt 版本是否正确
- 检查 CMake 配置
- 清理 build 目录重新构建

#### 运行时错误
- 检查日志文件 `logs/app.log`
- 使用 Qt Creator 调试器
- 检查内存泄漏

#### 样式问题
- 检查 QSS 语法
- 确认颜色值格式正确
- 使用 `QWidget::styleSheet()` 查看当前样式

### 6.3 性能优化

- 使用 `QElapsedTimer` 测量耗时
- 避免频繁的 UI 更新
- 使用数据缓存减少网络请求
- 启用 `QT_OPENGL` 加速渲染

---

## 附录

### A. 常用命令

```bash
# 清理构建
cmake --build . --target clean

# 重新构建
cmake --build . --target WealthPilot --clean-first

# 生成文档
doxygen Doxyfile

# 运行测试
ctest --output-on-failure
```

### B. 参考资源

- [Qt 6 文档](https://doc.qt.io/qt-6/)
- [CMake 文档](https://cmake.org/documentation/)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)

---

*版本：1.0.0 | 更新日期：2026-04-21*
