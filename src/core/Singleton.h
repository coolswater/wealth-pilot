/**
 * @file Singleton.h
 * @brief 线程安全的单例模板基类
 * @endcode
 */

#ifndef SINGLETON_H
#define SINGLETON_H

/**
 * @brief 单例模板类
 * @tparam T 派生类类型（CRTP模式）
 */
template<typename T>
class Singleton
{
public:
    /**
     * @brief 获取单例实例
     * @return T* 单例指针，永不返回 nullptr
     */
    static T* instance()
    {
        static T instance;
        return &instance;
    }

    /**
     * @brief 获取单例引用
     * @return T& 单例引用
     */
    static T& ref()
    {
        return *instance();
    }

protected:
    Singleton() = default;
    ~Singleton() = default;

    // 禁止拷贝和移动
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

#endif // SINGLETON_H
