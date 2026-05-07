/**
 * @file LayoutManager.h
 * @brief 布局管理器 - 多屏布局保存与恢复
 *
 * @details 提供布局管理功能：
 * - 窗口布局保存
 * - 多显示器支持
 * - 布局模板管理
 * - 布局切换
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef LAYOUTMANAGER_H
#define LAYOUTMANAGER_H

#include <QObject>
#include <QMap>
#include <QVector>
#include <QString>
#include <QRect>
#include <QSettings>

class QWidget;
class QMainWindow;
class QSplitter;

/**
 * @brief 窗口布局信息
 */
struct WindowLayout {
    QString name;               ///< 窗口名称
    QString className;          ///< 窗口类名
    QRect geometry;             ///< 几何信息
    bool visible = true;        ///< 是否可见
    bool maximized = false;     ///< 是否最大化
    int screenIndex = 0;        ///< 显示器索引
    QVariantMap customData;     ///< 自定义数据
};

/**
 * @brief 分割器布局信息
 */
struct SplitterLayout {
    QString name;               ///< 分割器名称
    QVector<int> sizes;         ///< 分割尺寸
    QByteArray state;           ///< 状态数据
};

/**
 * @brief 布局模板
 */
struct LayoutTemplate {
    QString id;                 ///< 模板ID
    QString name;               ///< 模板名称
    QString description;        ///< 描述
    QString category;           ///< 分类（交易/分析/监控）
    QVector<WindowLayout> windows;  ///< 窗口布局
    QVector<SplitterLayout> splitters; ///< 分割器布局
    bool isDefault = false;     ///< 是否默认模板
    QDateTime createTime;       ///< 创建时间
    QDateTime updateTime;       ///< 更新时间
};

/**
 * @brief 显示器信息
 */
struct MonitorInfo {
    int index;                  ///< 显示器索引
    QString name;               ///< 显示器名称
    QRect geometry;             ///< 几何信息
    QRect availableGeometry;    ///< 可用几何信息
    double dpi = 96.0;          ///< DPI
    bool isPrimary = false;     ///< 是否主显示器
};

/**
 * @brief 布局管理器
 */
class LayoutManager : public QObject
{
    Q_OBJECT

public:
    static LayoutManager* instance();

    /**
     * @brief 初始化布局管理器
     */
    bool initialize();

    /**
     * @brief 保存当前布局
     */
    bool saveCurrentLayout(const QString& name);

    /**
     * @brief 加载布局
     */
    bool loadLayout(const QString& templateId);

    /**
     * @brief 删除布局模板
     */
    bool deleteLayoutTemplate(const QString& templateId);

    /**
     * @brief 获取布局模板列表
     */
    QVector<LayoutTemplate> getLayoutTemplates() const;

    /**
     * @brief 获取布局模板
     */
    LayoutTemplate getLayoutTemplate(const QString& templateId) const;

    /**
     * @brief 设置默认布局
     */
    void setDefaultLayout(const QString& templateId);

    /**
     * @brief 应用布局到窗口
     */
    void applyLayout(QMainWindow* mainWindow, const LayoutTemplate& layout);

    /**
     * @brief 保存窗口布局
     */
    WindowLayout saveWindowLayout(QWidget* window) const;

    /**
     * @brief 恢复窗口布局
     */
    void restoreWindowLayout(QWidget* window, const WindowLayout& layout);

    /**
     * @brief 获取显示器信息
     */
    QVector<MonitorInfo> getMonitorInfo() const;

    /**
     * @brief 获取主显示器索引
     */
    int getPrimaryMonitorIndex() const;

    /**
     * @brief 导出布局模板
     */
    bool exportLayoutTemplate(const QString& templateId, const QString& filePath);

    /**
     * @brief 导入布局模板
     */
    bool importLayoutTemplate(const QString& filePath);

signals:
    void layoutSaved(const QString& templateId);
    void layoutLoaded(const QString& templateId);
    void layoutDeleted(const QString& templateId);
    void layoutChanged(const QString& templateId);

private:
    explicit LayoutManager(QObject* parent = nullptr);
    ~LayoutManager() override = default;

    // 布局保存/加载
    void saveToSettings(const LayoutTemplate& layout);
    LayoutTemplate loadFromSettings(const QString& templateId);

    // 分割器处理
    void saveSplitters(QWidget* parent, LayoutTemplate& layout);
    void restoreSplitters(QWidget* parent, const LayoutTemplate& layout);

    // 辅助方法
    QString generateTemplateId() const;
    void initDefaultTemplates();

    // 数据成员
    QMap<QString, LayoutTemplate> m_templates;
    QString m_currentLayoutId;
    QSettings* m_settings = nullptr;

    bool m_initialized = false;
};

#endif // LAYOUTMANAGER_H