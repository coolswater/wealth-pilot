# WealthPilot 代码风格指南

**版本**: 2.0.0  
**更新日期**: 2026-04-14

---

## 一、文件组织

### 1.1 文件命名

```
类名: MyService
头文件: MyService.h
实现文件: MyService.cpp
```

### 1.2 头文件结构

```cpp
/**
 * @file MyService.h
 * @brief 服务描述（一句话）
 * @author WealthPilot Team
 * @version 2.0.0
 * 
 * @details 详细描述（可选）
 */

#ifndef WEALTHPILOT_MODULE_MYSERVICE_H
#define WEALTHPILOT_MODULE_MYSERVICE_H

// 1. Qt 头文件
#include <QObject>
#include <QString>

// 2. STL 头文件
#include <memory>
#include <vector>

// 3. 项目头文件
#include "core/Singleton.h"
#include "utils/Result.h"

// 4. 命名空间
namespace WealthPilot::Module {

// 5. 前向声明
class OtherClass;

// 6. 类定义
class MyService : public QObject, public Singleton<MyService>
{
    Q_OBJECT
    friend class Singleton<MyService>;

public:
    // 公共方法
    
signals:
    // 信号
    
private slots:
    // 私有槽
    
private:
    // 私有成员
    
    struct Impl;
    std::unique_ptr<Impl> d;
};

} // namespace WealthPilot::Module

#endif // WEALTHPILOT_MODULE_MYSERVICE_H
```

---

## 二、命名规范

### 2.1 命名空间

```cpp
// 使用 PascalCase
namespace WealthPilot::Core {}
namespace WealthPilot::Models {}
```

### 2.2 类名

```cpp
// PascalCase
class CacheManager {};
class FuturesQuoteModel {};
```

### 2.3 函数名

```cpp
// camelCase
void fetchData();
QString getUserName();
bool isConnected() const;
```

### 2.4 变量名

```cpp
// 成员变量: m_ 前缀 + camelCase
int m_maxCount;
QString m_userName;

// 局部变量: camelCase
int totalCount;
QString userName;

// 常量: k 前缀 + PascalCase
const int kMaxRetryCount = 3;

// 枚举: PascalCase
enum class ErrorCode {
    Success,
    NetworkError
};
```

### 2.5 信号/槽

```cpp
// 信号: 过去式动词
signals:
    void dataReceived(const Data& data);
    void connectionLost();
    void errorOccurred(const Error& error);

// 槽: on + 事件名
private slots:
    void onDataReceived(const Data& data);
    void onConnectionLost();
```

---

## 三、PIMPL 模式

### 3.1 头文件

```cpp
class MyService {
public:
    MyService();
    ~MyService();  // 必须声明析构函数
    
private:
    struct Impl;
    std::unique_ptr<Impl> d;  // 使用 unique_ptr
};
```

### 3.2 实现文件

```cpp
// 在 cpp 文件开头定义 Impl
struct MyService::Impl {
    // 所有成员变量
    QString name;
    int count = 0;
    QTimer* timer = nullptr;
};

// 析构函数必须定义（unique_ptr 需要完整类型）
MyService::~MyService() = default;
```

---

## 四、错误处理

### 4.1 使用 Result<T>

```cpp
// 返回值可能失败的操作
Result<Data> fetchData(const QString& id) {
    if (id.isEmpty()) {
        return Result<Data>::err(
            ErrorCode::InvalidArgument,
            "ID cannot be empty"
        );
    }
    
    Data data = ...;
    return Result<Data>::ok(data);
}

// 调用方
auto result = service->fetchData("123");
if (result.isOk()) {
    Data data = result.unwrap();
} else {
    LOG_ERROR(result.errorMessage());
}
```

### 4.2 void 返回

```cpp
Result<void> initialize() {
    if (!connect()) {
        return Result<void>::err(
            ErrorCode::NetworkError,
            "Connection failed"
        );
    }
    return Result<void>::ok();
}
```

---

## 五、线程安全

### 5.1 标记线程安全

```cpp
/**
 * @brief 缓存管理器
 * @thread_safe 所有公共方法都是线程安全的
 */
class CacheManager {
public:
    // 使用 QMutexLocker 保护
    QVariant get(const QString& key) {
        QMutexLocker locker(&m_mutex);
        return m_cache.value(key);
    }
    
private:
    mutable QMutex m_mutex;
    QMap<QString, QVariant> m_cache;
};
```

### 5.2 信号槽连接

```cpp
// 跨线程连接
connect(sender, &Sender::signal,
        receiver, &Receiver::slot,
        Qt::QueuedConnection);  // 明确指定连接类型
```

---

## 六、日志规范

### 6.1 日志级别

```cpp
LOG_DEBUG("Detailed info for debugging");  // 仅 Debug 构建
LOG_INFO("Important state change");        // 正常运行信息
LOG_WARNING("Unexpected but handled");     // 警告
LOG_ERROR("Error occurred");               // 错误
```

### 6.2 日志格式

```cpp
// 使用 QString::arg() 格式化
LOG_INFO(QString("User %1 logged in from %2")
    .arg(userId)
    .arg(ipAddress));

// 避免直接拼接
// LOG_INFO("User " + userId + " logged in");  // 不推荐
```

---

## 七、代码组织

### 7.1 方法顺序

```cpp
class MyClass {
public:
    // 1. 构造/析构
    MyClass();
    ~MyClass();
    
    // 2. 公共接口
    void publicMethod();
    
    // 3. 静态方法
    static MyClass* create();
    
signals:
    // 4. 信号
    
public slots:
    // 5. 公共槽
    
protected:
    // 6. 保护方法
    
private slots:
    // 7. 私有槽
    
private:
    // 8. 私有方法
    void privateMethod();
    
    // 9. 成员变量
    int m_value;
    
    // 10. PIMPL
    struct Impl;
    std::unique_ptr<Impl> d;
};
```

---

## 八、最佳实践

### 8.1 使用 auto

```cpp
// 好
auto result = service->fetchData();
auto* widget = new QWidget(this);

// 避免
auto x = 5;  // 不清楚类型
```

### 8.2 使用 const

```cpp
// 成员函数
QString name() const;  // 不修改状态

// 参数
void process(const QString& input);

// 返回值
const QVector<Data>& allData() const;
```

### 8.3 使用 nullptr

```cpp
// 好
QObject* obj = nullptr;

// 避免
QObject* obj = NULL;   // 旧风格
QObject* obj = 0;      // 旧风格
```

### 8.4 使用范围 for

```cpp
// 好
for (const auto& item : items) {
    process(item);
}

// 避免
for (int i = 0; i < items.size(); ++i) {
    process(items[i]);
}
```
