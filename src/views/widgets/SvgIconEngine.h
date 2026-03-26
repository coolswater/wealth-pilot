// SvgIconEngine.h
#ifndef SVGICONENGINE_H
#define SVGICONENGINE_H

#include <QIconEngine>
#include <QColor>
#include <QByteArray>
#include <QSharedPointer>
#include <QMutex>
#include <QSvgRenderer>

class SvgIconEngine : public QIconEngine
{
public:
    // 构造方式1：从文件路径加载
    explicit SvgIconEngine(const QString& svgFilePath, const QColor& color = QColor());
    // 构造方式2：从二进制数据加载
    explicit SvgIconEngine(const QByteArray& svgData, const QColor& color = QColor());
    ~SvgIconEngine() override;

    // QIconEngine 接口实现
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state) override;
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override;
    QIconEngine* clone() const override;

    // 动态修改颜色（用于主题切换）
    void setColor(const QColor& color);
    QColor color() const;

    // 缓存管理
    static void clearCache();

    void preRender(const QSize& size);

private:
    QString m_svgFilePath;
    QByteArray m_svgData;
    QColor m_color;

    // 静态缓存：渲染器缓存，避免重复解析SVG
    static QMap<QString, QSharedPointer<QSvgRenderer>> s_rendererCache;
    static QMutex s_cacheMutex;
    static constexpr int MAX_CACHE_SIZE = 50;

    // 辅助方法
    QString cacheKey(const QSize& size, QIcon::Mode mode) const;
    QPixmap renderSvg(const QSize& size);
    QPixmap applyColorMask(const QPixmap& source, const QColor& color);
};

#endif // SVGICONENGINE_H
