    #ifndef SVGICONENGINE_H
#define SVGICONENGINE_H
#include <QIconEngine>
#include <QSvgRenderer>
#include <QPixmap>
#include <QMap>
#include <QMutex>
/**
     * @brief SVG图标引擎 - 高性能SVG渲染与动态着色
     *
     * 功能特性：
     * 1. 高性能SVG文件渲染
     * 2. 支持动态着色（适应股票状态颜色）
     * 3. 多级缓存机制（Pixmap Cache + LRU淘汰）
     * 4. 支持高DPI缩放
     * 5. 线程安全
     *
     * 性能优化：
     * - 使用QPixmapCache进行跨实例缓存共享
     * - 使用静态渲染器缓存避免重复解析SVG
     * - 支持预渲染常用尺寸
     */
class SvgIconEngine : public QIconEngine
{
public:
    /**
         * @brief 构造函数
         * @param svgFilePath SVG文件路径
         * @param color 着色颜色（可选，无效颜色表示使用原始颜色）
         */
    explicit SvgIconEngine(const QString& svgFilePath, const QColor& color = QColor());
    /**
         * @brief 从SVG数据构造
         * @param svgData SVG原始数据
         * @param color 着色颜色
         */
    explicit SvgIconEngine(const QByteArray& svgData, const QColor& color = QColor());
    // 拷贝构造
    SvgIconEngine(const SvgIconEngine& other);
    // 析构函数
    ~SvgIconEngine() override;
    // 重写QIconEngine接口
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine* clone() const override;
    // 设置着色颜色
    void setColor(const QColor& color);
    QColor color() const;
    // 设置SVG数据
    void setSvgData(const QByteArray& data);
    // 清除缓存
    static void clearCache();
    // 预渲染指定尺寸的图标（性能优化）
    void preRender(const QSize& size);
private:
    // 生成缓存键
    QString cacheKey(const QSize& size, QIcon::Mode mode) const;
    // 渲染SVG到Pixmap
    QPixmap renderSvg(const QSize& size);
    // 应用颜色遮罩
    QPixmap applyColorMask(const QPixmap& source, const QColor& color);
private:
    QString m_svgFilePath;
    QByteArray m_svgData;
    QColor m_color;
    // 静态渲染器缓存（线程安全）
    static QMap<QString, QSharedPointer<QSvgRenderer>> s_rendererCache;
    static QMutex s_cacheMutex;
    // 缓存统计
    static const int MAX_CACHE_SIZE = 100;
};
#endif // SVGICONENGINE_H
