# 代码注释规范

## 1. 文件头注释

每个源文件必须包含文件头注释：

```cpp
/**
 * @file FileName.h
 * @brief 简短描述（一句话）
 *
 * @details 详细描述：
 * - 功能点1
 * - 功能点2
 * - 使用说明
 *
 * @author WealthPilot Team
 * @version 1.0.0
 * @date 2026-05-11
 *
 * @note 重要说明
 * @warning 警告信息
 * @see 相关类或文件
 */
```

## 2. 类注释

```cpp
/**
 * @brief 类的简短描述
 *
 * @details 详细描述和使用示例
 *
 * @code
 * // 使用示例
 * auto* manager = CacheManager::instance();
 * manager->set("key", value);
 * @endcode
 *
 * @thread_safe 线程安全说明
 * @invariant 不变量说明
 */
class ClassName {
    // ...
};
```

## 3. 方法注释

```cpp
/**
 * @brief 方法简短描述
 *
 * @details 详细描述（可选）
 *
 * @param paramName 参数说明
 * @param anotherParam 另一个参数说明
 * @return 返回值说明
 *
 * @throws ExceptionType 异常说明
 *
 * @note 注意事项
 * @see 相关方法
 *
 * @example
 * @code
 * // 使用示例
 * int result = method(10, "test");
 * @endcode
 */
int method(int paramName, const QString& anotherParam);
```

## 4. 成员变量注释

```cpp
class Example {
    int m_count;        ///< 计数器
    QString m_name;     ///< 名称
    bool m_enabled;     ///< 是否启用

    /**
     * @brief 复杂成员的详细说明
     * @note 使用说明
     */
    QHash<QString, QVariant> m_cache;
};
```

## 5. 枚举注释

```cpp
/**
 * @brief 枚举描述
 */
enum class Status {
    Success,    ///< 成功
    Failed,     ///< 失败
    Pending,    ///< 待处理
    Cancelled   ///< 已取消
};
```

## 6. 代码块注释

```cpp
// ========== 区域标题 ==========

// 单行注释：说明下一行代码的目的
int result = calculate();

/*
 * 多行注释：
 * 用于解释复杂的代码逻辑
 * 或算法说明
 */
```

## 7. TODO 注释

```cpp
// TODO(author): 待实现功能描述
// FIXME: 需要修复的问题
// HACK: 临时解决方案，需要改进
// NOTE: 重要说明
// XXX: 危险或有问题的代码
```

## 8. 注释最佳实践

### 8.1 注释应该解释"为什么"，而不是"是什么"

```cpp
// ❌ 不好的注释
i++; // i 加 1

// ✅ 好的注释
i++; // 跳过头部标记字节
```

### 8.2 保持注释与代码同步

```cpp
// ❌ 注释与代码不一致
// 返回用户数量
int getUserCount() { return m_users.size() + 1; } // 实际返回数量+1

// ✅ 注释与代码一致
// 返回用户数量（包含当前用户）
int getUserCount() { return m_users.size() + 1; }
```

### 8.3 使用 Doxygen 标签

常用标签：
- `@brief` - 简短描述
- `@details` - 详细描述
- `@param` - 参数说明
- `@return` - 返回值说明
- `@throws` - 异常说明
- `@see` - 参见
- `@note` - 注意
- `@warning` - 警告
- `@code` / `@endcode` - 代码示例
- `@example` - 示例
- `@todo` - 待办
- `@deprecated` - 已废弃

### 8.4 中文注释规范

- 使用中文注释，清晰易懂
- 技术术语可保留英文
- 保持注释简洁，避免冗余

```cpp
/**
 * @brief 设置K线数据
 * @param data K线数据列表
 * @note 数据会被复制到内部存储
 */
void setKLineData(const QVector<KLineData>& data);
```

---

**文档版本**: 1.0.0  
**最后更新**: 2026-05-11
