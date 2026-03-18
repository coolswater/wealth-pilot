// 1. 页面基类 BasePage.h
#ifndef BASEPAGE_H
#define BASEPAGE_H

#include <QWidget>

class BasePage : public QWidget {
    Q_OBJECT
public:
    explicit BasePage(QWidget *parent = nullptr) : QWidget(parent) {}
    virtual ~BasePage() = default;

    /**
     * @brief 获取页面唯一标识符
     * @return 页面ID，用于工厂映射键值
     */
    virtual QString pageId() const = 0;

    /**
     * @brief 获取页面友好名称（用于UI显示）
     */
    virtual QString pageName() const { return pageId(); }

    /**
     * @brief 页面即将显示时触发（可在此加载数据）
     * @param params 上一个页面传递的参数
     */
    virtual void onPageActivated(const QVariantMap &params = QVariantMap()) {}

    /**
     * @brief 页面即将隐藏时触发（可在此保存状态）
     */
    virtual void onPageDeactivated() {}


    // 页面初始化
    virtual void initializePage() = 0;


    /**
     * @brief 判断页面是否允许多实例
     * @return false表示单例模式（默认），true表示每次创建新实例
     */
    virtual bool allowMultiInstance() const { return false; }

signals:
    /**
     * @brief 页面请求导航信号（解耦页面与导航器）
     * @param targetPageId 目标页面ID
     * @param params 传递参数
     */
    void requestNavigation(const QString &targetPageId, const QVariantMap &params);

    /**
     * @brief 页面状态变更通知
     */
    void pageStatusChanged(const QString &status);
};

#endif // BASEPAGE_H




