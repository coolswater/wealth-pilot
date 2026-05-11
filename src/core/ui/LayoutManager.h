/**
 * @file LayoutManager.h
 * @brief 窗口布局管理器 - 保存和恢复窗口布局
 *
 * @details 功能：
 * - 保存/恢复窗口位置和大小
 * - 保存/恢复分割器状态
 * - 保存/恢复选项卡顺序
 * - 多布局管理
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

#ifndef LAYOUTMANAGER_H
#define LAYOUTMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QHash>

class QMainWindow;
class QSplitter;
class QTabWidget;

/**
 * @brief 布局信息
 */
struct LayoutInfo {
    QString name;               ///< 布局名称
    QByteArray geometry;        ///< 窗口几何信息
    QByteArray state;           ///< 窗口状态
    QHash<QString, QByteArray> splitterStates;   ///< 分割器状态
    QHash<QString, QStringList> tabOrders;       ///< 选项卡顺序
};

/**
 * @brief 窗口布局管理器
 *
 * 提供布局管理功能：
 * - 自动保存/恢复
 * - 多布局支持
 * - 布局导入/导出
 */
class LayoutManager : public QObject {
    Q_OBJECT

public:
    static LayoutManager* instance();

    /**
     * @brief 初始化
     * @param mainWindow 主窗口
     */
    void initialize(QMainWindow* mainWindow);

    /**
     * @brief 保存当前布局
     * @param name 布局名称（空则使用默认）
     */
    void saveLayout(const QString& name = QString());

    /**
     * @brief 恢复布局
     * @param name 布局名称（空则使用默认）
     * @return 是否成功
     */
    bool restoreLayout(const QString& name = QString());

    /**
     * @brief 保存分割器状态
     * @param id 分割器ID
     * @param splitter 分割器对象
     */
    void saveSplitterState(const QString& id, QSplitter* splitter);

    /**
     * @brief 恢复分割器状态
     * @param id 分割器ID
     * @param splitter 分割器对象
     */
    void restoreSplitterState(const QString& id, QSplitter* splitter);

    /**
     * @brief 保存选项卡顺序
     * @param id 选项卡ID
     * @param tabWidget 选项卡控件
     */
    void saveTabOrder(const QString& id, QTabWidget* tabWidget);

    /**
     * @brief 恢复选项卡顺序
     * @param id 选项卡ID
     * @param tabWidget 选项卡控件
     */
    void restoreTabOrder(const QString& id, QTabWidget* tabWidget);

    /**
     * @brief 获取所有布局名称
     */
    QStringList getLayoutNames() const;

    /**
     * @brief 删除布局
     */
    void deleteLayout(const QString& name);

    /**
     * @brief 导出布局到文件
     */
    bool exportLayout(const QString& name, const QString& filePath);

    /**
     * @brief 从文件导入布局
     */
    bool importLayout(const QString& filePath);

    /**
     * @brief 设置自动保存
     * @param enabled 是否启用
     */
    void setAutoSave(bool enabled);

    /**
     * @brief 注册分割器
     */
    void registerSplitter(const QString& id, QSplitter* splitter);

    /**
     * @brief 注册选项卡
     */
    void registerTabWidget(const QString& id, QTabWidget* tabWidget);

signals:
    /**
     * @brief 布局保存完成
     */
    void layoutSaved(const QString& name);

    /**
     * @brief 布局恢复完成
     */
    void layoutRestored(const QString& name);

private slots:
    void onAboutToQuit();

private:
    explicit LayoutManager(QObject* parent = nullptr);
    ~LayoutManager() override;

    QString layoutKey(const QString& name) const;

    QMainWindow* m_mainWindow = nullptr;
    QHash<QString, LayoutInfo> m_layouts;
    QHash<QString, QSplitter*> m_splitters;
    QHash<QString, QTabWidget*> m_tabWidgets;
    QString m_currentLayout;
    bool m_autoSave = true;
};

#endif // LAYOUTMANAGER_H
