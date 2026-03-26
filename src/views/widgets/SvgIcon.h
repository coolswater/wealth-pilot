    #ifndef SVGICON_H
#define SVGICON_H
#include <QIcon>
#include <QColor>
#include <QString>
#include "SvgIconEngine.h"
#include "src/core/ThemeManager.h"
/**
     * @brief SVG图标便捷工具类
     *
     * 提供更简洁的API来创建和使用SVG图标
     * 支持主题自动适配
     */
class SvgIcon
{
public:
    /**
         * @brief 创建SVG图标
         * @param path SVG文件路径
         * @param color 着色颜色（可选）
         * @return QIcon对象
         */
    static QIcon icon(const QString& path, const QColor& color = QColor());
    /**
         * @brief 创建自适应主题颜色的SVG图标
         * @param path SVG文件路径
         * @param role 颜色角色（foreground, accent, rise, fall等）
         * @return QIcon对象
         */
    static QIcon themedIcon(const QString& path, const QString& colorRole);
    /**
         * @brief 创建上涨状态图标（红色）
         */
    static QIcon riseIcon(const QString& path);
    /**
         * @brief 创建下跌状态图标（绿色）
         */
    static QIcon fallIcon(const QString& path);
    /**
         * @brief 批量创建图标并预渲染
         * @param paths SVG文件路径列表
         * @param size 预渲染尺寸
         * @return 图标列表
         */
    static QList<QIcon> batchCreate(const QStringList& paths, const QSize& size = QSize(32, 32));
    /**
         * @brief 清除图标缓存
         */
    static void clearCache();
};
#endif // SVGICON_H
