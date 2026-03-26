    #include "SvgIconEngine.h"
#include <QPainter>
#include <QFile>
#include <QPixmapCache>
#include <QDebug>
#include <QApplication>
#include <QScreen>

// 静态成员初始化
QMap<QString, QSharedPointer<QSvgRenderer>> SvgIconEngine::s_rendererCache;
QMutex SvgIconEngine::s_cacheMutex;
SvgIconEngine::SvgIconEngine(const QString& svgFilePath, const QColor& color)
    : m_svgFilePath(svgFilePath)
    , m_color(color)
{
    // 加载SVG文件数据
    QFile file(svgFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        m_svgData = file.readAll();
        file.close();
    }
}
SvgIconEngine::SvgIconEngine(const QByteArray& svgData, const QColor& color)
    : m_svgData(svgData)
    , m_color(color)
{
}
SvgIconEngine::SvgIconEngine(const SvgIconEngine& other)
    : QIconEngine(other)
    , m_svgFilePath(other.m_svgFilePath)
    , m_svgData(other.m_svgData)
    , m_color(other.m_color)
{
}
SvgIconEngine::~SvgIconEngine()
{
}
void SvgIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);
    // 获取设备像素比
    qreal dpr = painter->device()->devicePixelRatioF();
    QSize size = rect.size() * dpr;
    // 获取或渲染Pixmap
    QPixmap pix = pixmap(size, mode, state);
    if (!pix.isNull()) {
        painter->drawPixmap(rect, pix);
    }
}
QPixmap SvgIconEngine::pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(mode);
    Q_UNUSED(state);
    // 生成缓存键
    QString key = cacheKey(size, mode);
    // 尝试从全局缓存获取
    QPixmap cachedPixmap;
    if (QPixmapCache::find(key, &cachedPixmap)) {
        return cachedPixmap;
    }
    // 渲染SVG
    QPixmap pix = renderSvg(size);
    if (pix.isNull()) {
        return QPixmap();
    }
    // 如果指定了颜色，应用颜色遮罩
    if (m_color.isValid()) {
        pix = applyColorMask(pix, m_color);
    }
    // 存入缓存
    QPixmapCache::insert(key, pix);
    return pix;
}
QIconEngine* SvgIconEngine::clone() const
{
    return new SvgIconEngine(*this);
}
void SvgIconEngine::setColor(const QColor& color)
{
    if (m_color != color) {
        m_color = color;
        // 颜色变化需要清除相关缓存
        // 这里不主动清除，依靠PixmapCache的LRU机制自动管理
    }
}
QColor SvgIconEngine::color() const
{
    return m_color;
}
void SvgIconEngine::setSvgData(const QByteArray& data)
{
    m_svgData = data;
    m_svgFilePath.clear();
}
void SvgIconEngine::clearCache()
{
    QMutexLocker locker(&s_cacheMutex);
    s_rendererCache.clear();
    QPixmapCache::clear();
}
void SvgIconEngine::preRender(const QSize& size)
{
    // 预渲染并缓存指定尺寸
    pixmap(size, QIcon::Normal, QIcon::Off);
}
QString SvgIconEngine::cacheKey(const QSize& size, QIcon::Mode mode) const
{
    // 生成唯一缓存键
    QString colorStr = m_color.isValid() ? m_color.name(QColor::HexArgb) : "original";
    return QString("svg_icon_%1_%2x%3_%4_%5")
        .arg(m_svgFilePath.isEmpty() ? QString::number(qHash(m_svgData)) : m_svgFilePath)
        .arg(size.width())
        .arg(size.height())
        .arg(colorStr)
        .arg(static_cast<int>(mode));
}
QPixmap SvgIconEngine::renderSvg(const QSize& size)
{
    if (m_svgData.isEmpty()) {
        return QPixmap();
    }
    // 获取或创建渲染器
    QSharedPointer<QSvgRenderer> renderer;
    QString cacheKey = m_svgFilePath.isEmpty()
                           ? QString::number(qHash(m_svgData))
                           : m_svgFilePath;
    {
        QMutexLocker locker(&s_cacheMutex);
        if (s_rendererCache.contains(cacheKey)) {
            renderer = s_rendererCache[cacheKey];
        } else {
            renderer = QSharedPointer<QSvgRenderer>::create(m_svgData);
            if (renderer->isValid()) {
                // 实现简单的LRU淘汰
                if (s_rendererCache.size() >= MAX_CACHE_SIZE) {
                    s_rendererCache.remove(s_rendererCache.firstKey());
                }
                s_rendererCache[cacheKey] = renderer;
            }
        }
    }
    if (!renderer || !renderer->isValid()) {
        return QPixmap();
    }
    // 创建高DPI Pixmap
    qreal dpr = qApp->devicePixelRatio();
    QPixmap pix(size * dpr);
    pix.setDevicePixelRatio(dpr);
    pix.fill(Qt::transparent);
    // 渲染SVG
    QPainter painter(&pix);
    renderer->render(&painter);
    painter.end();
    return pix;
}
QPixmap SvgIconEngine::applyColorMask(const QPixmap& source, const QColor& color)
{
    if (source.isNull()) {
        return source;
    }
    // 创建遮罩图像
    QPixmap result(source.size());
    result.setDevicePixelRatio(source.devicePixelRatioF());
    result.fill(Qt::transparent);
    QPainter painter(&result);
    // 先绘制原图（获取形状）
    painter.drawPixmap(0, 0, source);
    // 使用CompositionMode_SourceIn进行着色
    // 这会保留原图的透明度，但替换颜色
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(result.rect(), color);
    painter.end();
    return result;
}
