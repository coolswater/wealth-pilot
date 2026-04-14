# WealthPilot 开发指南

## 目录

1. [开发环境搭建](#开发环境搭建)
2. [项目结构](#项目结构)
3. [编码规范](#编码规范)
4. [开发流程](#开发流程)
5. [调试技巧](#调试技巧)
6. [发布流程](#发布流程)

---

## 开发环境搭建

### 必需软件

1. **Qt 6.10.2**
   - 下载地址: https://www.qt.io/download
   - 安装组件: MinGW 64-bit, Qt Creator

2. **CMake 3.16+**
   - 下载地址: https://cmake.org/download/

3. **Git**
   - 下载地址: https://git-scm.com/download

4. **Visual Studio Code** (可选)
   - 推荐插件: C/C++, CMake Tools

### 克隆项目

```bash
git clone https://github.com/wealthpilot/wealth-pilot.git
cd wealth-pilot
```

### 编译项目

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

### 运行项目

```bash
./WealthPilot
```

---

## 项目结构

```
wealth-pilot/
├── src/                    # 源代码
│   ├── core/              # 核心模块
│   │   ├── ServiceLocator.h/cpp
│   │   ├── EnvironmentConfig.h/cpp
│   │   ├── CacheManager.h/cpp
│   │   ├── DatabaseManager.h/cpp
│   │   └── AsyncTaskManager.h/cpp
│   │
│   ├── plugins/           # 插件系统
│   │   ├── IPlugin.h
│   │   ├── PluginLoader.h/cpp
│   │   ├── ICTPPlugin.h
│   │   ├── IAIPlugin.h
│   │   ├── CTPPlugin.h/cpp
│   │   └── AIPlugin.h/cpp
│   │
│   ├── ui/                # UI组件库
│   │   └── components/
│   │       ├── UIComponents.h
│   │       ├── ThemeEngine.h/cpp
│   │       └── KLineChart.h
│   │
│   ├── network/           # 网络模块
│   │   └── NetworkCache.h/cpp
│   │
│   ├── utils/             # 工具模块
│   │   ├── TechnicalIndicators.h/cpp
│   │   └── Logger.h/cpp
│   │
│   ├── services/          # 服务层
│   │   ├── AIService.h/cpp
│   │   └── CTPService.h/cpp
│   │
│   └── views/             # 视图层
│       ├── mainWindow/
│       ├── widgets/
│       └── pages/
│
├── tests/                 # 测试代码
│   ├── TestServiceLocator.cpp
│   ├── TestCacheManager.cpp
│   ├── TestPluginLoader.cpp
│   └── PerformanceTest.cpp
│
├── docs/                  # 文档
│   ├── OPTIMIZATION_ARCHITECTURE.md
│   ├── OPTIMIZATION_REPORT.md
│   ├── REFACTORING_REPORT.md
│   ├── API_DOCUMENTATION.md
│   └── USER_MANUAL.md
│
├── resources/             # 资源文件
│   ├── icons/
│   ├── images/
│   ├── fonts/
│   └── style/
│
├── external/              # 第三方库
├── scripts/               # 构建脚本
├── CMakeLists.txt         # CMake配置
└── README.md              # 项目说明
```

---

## 编码规范

### 命名规范

#### 类名
- 使用PascalCase
- 以大写字母开头
- 示例: `ServiceLocator`, `CacheManager`

#### 函数名
- 使用camelCase
- 以小写字母开头
- 示例: `initialize()`, `getMarketData()`

#### 变量名
- 成员变量: `m_` 前缀 + camelCase
- 示例: `m_cache`, `m_networkManager`
- 局部变量: camelCase
- 示例: `result`, `dataList`

#### 常量
- 使用UPPER_CASE
- 示例: `MAX_CACHE_SIZE`, `DEFAULT_TIMEOUT`

#### 枚举
- 使用PascalCase
- 示例: `enum class CacheLevel { L1_Memory, L2_Disk }`

### 代码风格

#### 头文件
```cpp
/**
 * @file FileName.h
 * @brief 文件描述
 * @author WealthPilot Team
 * @version 2.0.0
 */

#ifndef FILENAME_H
#define FILENAME_H

#include <QObject>

class ClassName : public QObject
{
    Q_OBJECT

public:
    explicit ClassName(QObject* parent = nullptr);
    ~ClassName() override;

private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

#endif // FILENAME_H
```

#### 实现文件
```cpp
/**
 * @file FileName.cpp
 * @brief 文件描述
 */

#include "FileName.h"

struct ClassName::Impl {
    // 私有实现
};

ClassName::ClassName(QObject* parent)
    : QObject(parent)
    , d(std::make_unique<Impl>())
{
}

ClassName::~ClassName()
{
}
```

#### 函数注释
```cpp
/**
 * @brief 函数描述
 * @param paramName 参数描述
 * @return 返回值描述
 */
ReturnType functionName(ParamType paramName);
```

### 最佳实践

#### 使用智能指针
```cpp
// 推荐
std::unique_ptr<Impl> d;
std::shared_ptr<Data> data;

// 避免
Impl* d;
Data* data;
```

#### 使用RAII
```cpp
// 推荐
{
    QMutexLocker locker(&m_mutex);
    // 访问共享资源
}
// 自动解锁

// 避免
m_mutex.lock();
// 访问共享资源
m_mutex.unlock();
```

#### 使用const
```cpp
// 推荐
const QString& getName() const;
void processData(const QMap<QString, QVariant>& data);

// 避免不必要的拷贝
QString getName(); // 不推荐
```

#### 使用PIMPL模式
```cpp
// 头文件
class ClassName {
private:
    struct Impl;
    std::unique_ptr<Impl> d;
};

// 实现文件
struct ClassName::Impl {
    // 私有成员
};
```

---

## 开发流程

### 1. 创建新功能

#### 步骤1: 创建分支
```bash
git checkout -b feature/new-feature
```

#### 步骤2: 编写代码
- 遵循编码规范
- 添加必要注释
- 编写单元测试

#### 步骤3: 编译测试
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
ctest
```

#### 步骤4: 提交代码
```bash
git add .
git commit -m "feat: add new feature"
git push origin feature/new-feature
```

#### 步骤5: 创建Pull Request
- 在GitHub创建PR
- 等待代码审查
- 合并到主分支

### 2. 创建新插件

#### 步骤1: 定义接口
```cpp
// src/plugins/IMyPlugin.h
class IMyPlugin : public IPlugin
{
    Q_OBJECT
public:
    virtual void myFunction() = 0;
};

Q_DECLARE_INTERFACE(IMyPlugin, "com.wealthpilot.IMyPlugin")
```

#### 步骤2: 实现插件
```cpp
// src/plugins/MyPlugin.h
class MyPlugin : public QObject, public IMyPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.wealthpilot.IMyPlugin")
    Q_INTERFACES(IMyPlugin)
public:
    void myFunction() override;
};
```

#### 步骤3: 注册插件
```cpp
// 在ApplicationInitializer中注册
registerModule("MyPlugin", InitPhase::Plugins, []() {
    return PluginLoader::instance().loadPlugin("MyPlugin");
});
```

### 3. 添加新页面

#### 步骤1: 创建页面类
```cpp
// src/views/pages/MyPage.h
class MyPage : public BasePage
{
    Q_OBJECT
public:
    explicit MyPage(QWidget* parent = nullptr);
private:
    void setupUI();
};
```

#### 步骤2: 注册页面
```cpp
// 在MainWindow中注册
void MainWindow::createPages()
{
    d->pageCache["mypage"] = nullptr; // 懒加载
}
```

#### 步骤3: 实现懒加载
```cpp
QWidget* MainWindow::getPage(const QString& pageId)
{
    if (pageId == "mypage") {
        auto page = new MyPage(this);
        d->pageCache["mypage"] = page;
        return page;
    }
}
```

---

## 调试技巧

### 使用日志

```cpp
#include "utils/Logger.h"

LOG_DEBUG("Debug message");
LOG_INFO("Info message");
LOG_WARNING("Warning message");
LOG_ERROR("Error message");
```

### 使用断点

1. 在Qt Creator中设置断点
2. 点击调试按钮
3. 查看变量值

### 使用性能分析

```cpp
#include <QElapsedTimer>

QElapsedTimer timer;
timer.start();

// 执行代码

qDebug() << "Elapsed:" << timer.elapsed() << "ms";
```

### 内存泄漏检测

```cpp
// 使用Valgrind (Linux)
valgrind --leak-check=full ./WealthPilot

// 使用Visual Studio (Windows)
// 调试 -> 性能探查器 -> 内存使用情况
```

---

## 发布流程

### 1. 版本号规范

使用语义化版本号: `MAJOR.MINOR.PATCH`

- MAJOR: 重大更新，不兼容的API修改
- MINOR: 新增功能，向后兼容
- PATCH: Bug修复，向后兼容

### 2. 编译发布版本

```bash
mkdir build-release
cd build-release
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
```

### 3. 打包发布

```bash
# Windows
windeployqt WealthPilot.exe

# macOS
macdeployqt WealthPilot.app

# Linux
linuxdeployqt WealthPilot
```

### 4. 创建发布说明

```markdown
# WealthPilot v2.0.0

## 新增功能
- 插件系统
- AI智能分析
- 技术指标计算

## 改进
- 性能优化
- UI改进

## Bug修复
- 修复连接问题
- 修复内存泄漏
```

### 5. 发布到GitHub

1. 创建Release标签
2. 上传安装包
3. 发布Release Notes

---

## 贡献指南

### 提交规范

使用约定式提交:

- `feat:` 新功能
- `fix:` Bug修复
- `docs:` 文档更新
- `style:` 代码格式
- `refactor:` 重构
- `test:` 测试
- `chore:` 构建/工具

示例:
```
feat: add AI analysis feature
fix: resolve memory leak in cache manager
docs: update API documentation
```

### 代码审查

- 所有代码必须经过审查
- 至少一位审查者批准
- 通过所有测试

---

## 联系方式

- **项目主页**: https://github.com/wealthpilot/wealth-pilot
- **问题反馈**: https://github.com/wealthpilot/wealth-pilot/issues
- **邮箱**: dev@wealthpilot.com

---

**WealthPilot Team**
