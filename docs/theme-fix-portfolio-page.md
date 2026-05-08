# PortfolioPage 主题切换修复总结

## 修改日期
2026-05-08

## 问题描述
PortfolioPage 页面在主题切换时没有响应，导致颜色不会随主题变化而更新。

## 解决方案

### 1. 添加主题监听器注册
在 `PortfolioPage` 构造函数中添加了对 `ThemeEngine::themeChanged` 信号的监听：

```cpp
// 注册主题监听器
connect(&ThemeEngine::instance(), &ThemeEngine::themeChanged,
        this, &PortfolioPage::updateTheme);
```

### 2. 创建 updateTheme() 方法
新增 `updateTheme()` 槽函数，用于响应主题切换事件：

```cpp
void PortfolioPage::updateTheme()
{
    // 获取当前主题配置
    auto& themeEngine = ThemeEngine::instance();
    auto themeConfig = themeEngine.themeConfig();

    // 更新页面背景色
    setStyleSheet(QString("background-color: %1;").arg(themeConfig.bgPrimary));

    // 更新卡片样式
    QString cardStyle = QString(R"(
        QFrame {
            background-color: %1;
            border-radius: 8px;
        }
    )").arg(themeConfig.bgElevated);

    // 更新所有UI组件的颜色...
}
```

### 3. 替换硬编码颜色
将所有硬编码的颜色值替换为从 `ThemeEngine` 获取的主题配置：

- 页面背景色：`themeConfig.bgPrimary`
- 卡片背景色：`themeConfig.bgElevated`
- 文本颜色：`themeConfig.textPrimary`, `themeConfig.textSecondary`, `themeConfig.textTertiary`
- 边框颜色：`themeConfig.border`
- 功能色：`themeConfig.primary`, `themeConfig.success`, `themeConfig.danger`, `themeConfig.warning`

### 4. 在 setupUI() 中调用 updateTheme()
在 `setupUI()` 方法末尾添加了 `updateTheme()` 调用，确保页面初始化时应用正确的主题：

```cpp
void PortfolioPage::setupUI()
{
    // ... 创建UI组件 ...

    // 应用主题样式
    updateTheme();
}
```

## 修改的文件

### PortfolioPage.h
- 添加 `updateTheme()` 槽函数声明
- 路径：`D:\C++\wealth-pilot\src\views\portfolio\PortfolioPage.h`

### PortfolioPage.cpp
- 添加 `#include "ui/components/ThemeEngine.h"`
- 在构造函数中注册主题监听器
- 实现 `updateTheme()` 方法
- 移除 `setupUI()` 中的硬编码背景色设置
- 在 `setupUI()` 末尾调用 `updateTheme()`
- 路径：`D:\C++\wealth-pilot\src\views\portfolio\PortfolioPage.cpp`

## 技术细节

### ThemeEngine 使用
项目使用 `ThemeEngine` 单例来管理主题：
- `ThemeEngine::instance()` - 获取单例实例
- `ThemeEngine::themeChanged` - 主题切换信号
- `ThemeEngine::themeConfig()` - 获取当前主题配置

### ThemeConfig 结构
主题配置包含以下颜色属性：
- `bgPrimary` - 主背景色
- `bgElevated` - 提升背景色（卡片等）
- `bgSurface` - 表面背景色
- `textPrimary` - 主文本色
- `textSecondary` - 次要文本色
- `textTertiary` - 三级文本色
- `border` - 边框色
- `primary` - 主色调
- `success` - 成功色（涨）
- `danger` - 危险色（跌）
- `warning` - 警告色
- `info` - 信息色

## 测试建议

1. **编译测试**：确保代码能够正常编译
2. **运行时测试**：
   - 启动应用程序
   - 导航到持仓页面
   - 切换主题（深色/浅色/护眼）
   - 验证所有UI元素的颜色是否正确更新
3. **边界测试**：
   - 在不同主题下启动应用
   - 快速切换主题
   - 检查是否有内存泄漏或崩溃

## 注意事项

1. **文件编码**：所有修改保持了 UTF-8 编码（带 BOM）
2. **性能考虑**：`updateTheme()` 方法在主题切换时会被调用，应避免耗时操作
3. **扩展性**：如果将来添加新的UI组件，需要在 `updateTheme()` 中添加相应的样式更新代码

## 相关文件

- `D:\C++\wealth-pilot\src\ui\components\ThemeEngine.h` - 主题引擎定义
- `D:\C++\wealth-pilot\src\ui\ThemeManager.h` - 主题管理器
- `D:\C++\wealth-pilot\src\core\config\Tokens.h` - 设计令牌系统
