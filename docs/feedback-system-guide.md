# 用户反馈系统使用指南

## 概述

WealthPilot 用户反馈系统提供统一的用户交互反馈机制，包括：
- 信息提示（Toast、对话框、系统通知）
- 进度反馈
- 反馈历史记录
- 国际化支持

## 快速开始

### 1. 基础用法

```cpp
#include "core/feedback/UserFeedbackManager.h"

// 显示信息
WealthPilot::showInfo("提示", "操作已完成");

// 显示警告
WealthPilot::showWarning("警告", "数据可能不完整");

// 显示错误
WealthPilot::showError("错误", "网络连接失败");

// 显示成功
WealthPilot::showSuccess("成功", "订单已提交");
```

### 2. 使用宏

```cpp
// 简洁的宏方式
FEEDBACK_INFO("提示", "这是一条信息");
FEEDBACK_WARNING("警告", "这是一条警告");
FEEDBACK_ERROR("错误", "这是一条错误");
FEEDBACK_SUCCESS("成功", "操作成功");
```

### 3. 进度反馈

```cpp
// 开始进度
ProgressConfig config;
config.title = "加载数据";
config.maximum = 100;
UserFeedbackManager::instance()->beginProgress("load_data", config);

// 更新进度
UserFeedbackManager::instance()->updateProgress("load_data", 50, "处理中...");

// 结束进度
UserFeedbackManager::instance()->endProgress("load_data", true, "加载完成");
```

### 4. 确认对话框

```cpp
// 删除确认
bool confirmed = WealthPilot::showConfirm(
    "确认删除",
    "确定要删除此项吗？此操作不可撤销。",
    "删除",
    "取消"
);

if (confirmed) {
    // 执行删除操作
}
```

### 5. 输入对话框

```cpp
QString name = UserFeedbackManager::instance()->showInput(
    "输入名称",
    "请输入项目名称：",
    "默认名称"
);

if (!name.isEmpty()) {
    // 使用输入的名称
}
```

## 反馈类型

### FeedbackType
- `Info`: 信息提示（蓝色）
- `Warning`: 警告提示（橙色）
- `Error`: 错误提示（红色）
- `Success`: 成功提示（绿色）

### FeedbackLevel
- `Toast`: 轻提示，自动消失
- `Dialog`: 对话框，需要用户确认
- `Notification`: 系统通知

## Toast 控件

### 自定义 Toast 位置

```cpp
#include "ui/components/ToastWidget.h"

// 设置 Toast 显示位置
WealthPilot::UI::ToastManager::instance()->setPosition(
    WealthPilot::UI::ToastPosition::TopRight
);

// 可选位置：
// - TopLeft, TopCenter, TopRight
// - BottomLeft, BottomCenter, BottomRight
// - Center
```

### Toast 便捷函数

```cpp
using namespace WealthPilot::UI;

showInfoToast("信息", "这是一条信息");
showWarningToast("警告", "这是一条警告");
showErrorToast("错误", "这是一条错误");
showSuccessToast("成功", "操作成功");
```

## 国际化支持

### 使用翻译函数

```cpp
#include "core/feedback/FeedbackTranslations.h"

using namespace WealthPilot::Feedback;

// 使用预定义的翻译
showInfo(FeedbackTranslations::operationSuccess(), "数据已保存");

// 带参数的翻译
showWarning(
    FeedbackTranslations::riskWarning(),
    FeedbackTranslations::highRiskOperation()
);

// 格式化翻译
QString message = FeedbackTranslations::confirmDeleteItem("项目A");
```

### 可翻译字符串

所有用户可见的字符串都应使用 `FeedbackTranslations` 类中的函数，以便支持多语言。

## 最佳实践

### 1. 选择合适的反馈级别

```cpp
// ✅ 好的做法：根据重要性选择级别
showInfo("提示", "数据已刷新", FeedbackLevel::Toast, 2000);  // 轻提示
showError("错误", "无法连接服务器", FeedbackLevel::Dialog);   // 重要错误用对话框

// ❌ 不好的做法：所有都用对话框
showInfo("提示", "数据已刷新", FeedbackLevel::Dialog);  // 太打扰用户
```

### 2. 提供有意义的反馈信息

```cpp
// ✅ 好的做法：具体明确
showError("登录失败", "用户名或密码错误，请重试");

// ❌ 不好的做法：模糊不清
showError("错误", "操作失败");
```

### 3. 使用进度反馈

```cpp
// ✅ 好的做法：长时间操作显示进度
void loadLargeData() {
    ProgressConfig config;
    config.title = "加载数据";
    config.maximum = totalItems;
    UserFeedbackManager::instance()->beginProgress("load", config);

    for (int i = 0; i < totalItems; ++i) {
        // 处理数据
        UserFeedbackManager::instance()->updateProgress(
            "load", i, QString("处理第 %1/%2 项").arg(i+1).arg(totalItems)
        );

        // 检查是否取消
        if (UserFeedbackManager::instance()->isProgressCancelled("load")) {
            break;
        }
    }

    UserFeedbackManager::instance()->endProgress("load", true);
}
```

### 4. 记录重要操作

```cpp
// 反馈系统会自动记录历史
// 可以查询和导出历史记录
auto history = UserFeedbackManager::instance()->getHistory(100);
QString report = UserFeedbackManager::instance()->exportHistory("text");
```

## 集成示例

参考 `src/examples/FeedbackIntegrationExample.h` 查看各种场景的完整示例。

## 测试

运行单元测试验证功能：

```bash
# 构建测试
cmake --build . --target UserFeedbackManagerTest

# 运行测试
./UserFeedbackManagerTest
```

## 常见问题

### Q: Toast 不显示？
A: 确保 ToastManager 已初始化，并且应用有活动窗口。

### Q: 如何自定义 Toast 样式？
A: 修改 `ToastWidget.cpp` 中的 `applyStyle()` 方法。

### Q: 如何添加新的翻译？
A: 在 `FeedbackTranslations.h` 中添加新的静态函数，并更新翻译文件。

## 相关文件

- `src/core/feedback/UserFeedbackManager.h` - 反馈管理器
- `src/core/feedback/UserFeedbackManager.cpp` - 实现文件
- `src/core/feedback/FeedbackTranslations.h` - 国际化支持
- `src/ui/components/ToastWidget.h` - Toast 控件
- `src/ui/components/ToastWidget.cpp` - Toast 实现
- `src/examples/FeedbackIntegrationExample.h` - 集成示例
- `tests/UserFeedbackManagerTest.cpp` - 单元测试