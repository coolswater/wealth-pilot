// SvgIconEngine.cpp
#include "SvgIconEngine.h"
#include <QFile>
#include <QPixmapCache>
#include <QPainter>
#include <QGuiApplication>

// 静态成员初始化
QMap<QString, QSharedPointer<QSvgRenderer>> SvgIconEngine::s_rendererCache;
QMutex SvgIconEngine::s_cacheMutex;

SvgIconEngine::SvgIconEngine(const QString& svgFilePath, const QColor& color)
    : m_svgFilePath(svgFilePath), m_color(color)
{
    QFile file(svgFilePath);
    if (file.open(QIODevice::ReadOnly)) {
        m_svgData = file.readAll();
        file.close();
    }
}

SvgIconEngine::SvgIconEngine(const QByteArray& svgData, const QColor& color)
    : m_svgData(svgData), m_color(color)
{
}

SvgIconEngine::~SvgIconEngine() = default;

void SvgIconEngine::paint(QPainter* painter, const QRect& rect, QIcon::Mode mode, QIcon::State state)
{
    Q_UNUSED(state);
    QPixmap pix = pixmap(rect.size(), mode, state);
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
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        return cached;
    }

    QPixmap pix = renderSvg(size);
    if (pix.isNull()) {
        return QPixmap();
    }

    // 应用颜色遮罩（如果指定了颜色）
    if (m_color.isValid()) {
        pix = applyColorMask(pix, m_color);
    }

    QPixmapCache::insert(key, pix);
    return pix;
}

QIconEngine* SvgIconEngine::clone() const
{
    return new SvgIconEngine(*this);
}

void SvgIconEngine::setColor(const QColor& color)
{
    m_color = color;
    // 颜色变化后不清除缓存，由缓存键自然区分（颜色包含在缓存键中）
}

QColor SvgIconEngine::color() const
{
    return m_color;
}

void SvgIconEngine::clearCache()
{
    QMutexLocker locker(&s_cacheMutex);
    s_rendererCache.clear();
    QPixmapCache::clear();
}

void SvgIconEngine::preRender(const QSize &size)
{
    // 提前渲染并缓存该尺寸的图标
    renderSvg(size);
}

QString SvgIconEngine::cacheKey(const QSize& size, QIcon::Mode mode) const
{
    QString colorStr = m_color.isValid() ? m_color.name(QColor::HexArgb) : "original";
    // 使用文件路径或数据哈希作为源标识
    QString sourceId = m_svgFilePath.isEmpty()
                           ? QString::number(qHash(m_svgData))
                           : m_svgFilePath;
    return QString("svg_%1_%2x%3_%4_%5")
        .arg(sourceId)
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

    // 获取或创建渲染器（带LRU淘汰）
    QSharedPointer<QSvgRenderer> renderer;
    QString sourceKey = m_svgFilePath.isEmpty()
                            ? QString::number(qHash(m_svgData))
                            : m_svgFilePath;
    {
        QMutexLocker locker(&s_cacheMutex);
        if (s_rendererCache.contains(sourceKey)) {
            renderer = s_rendererCache[sourceKey];
        } else {
            renderer = QSharedPointer<QSvgRenderer>::create(m_svgData);
            if (renderer->isValid()) {
                if (s_rendererCache.size() >= MAX_CACHE_SIZE) {
                    // 简单淘汰第一个（LRU模拟，实际可用迭代器）
                    auto it = s_rendererCache.begin();
                    s_rendererCache.erase(it);
                }
                s_rendererCache[sourceKey] = renderer;
            }
        }
    }

    if (!renderer || !renderer->isValid()) {
        return QPixmap();
    }

    // 渲染到高DPI pixmap
    qreal dpr = qApp->devicePixelRatio();
    QPixmap pix(size * dpr);
    pix.setDevicePixelRatio(dpr);
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    renderer->render(&painter);
    painter.end();

    return pix;
}

QPixmap SvgIconEngine::applyColorMask(const QPixmap& source, const QColor& color)
{
    QPixmap result(source.size());
    result.setDevicePixelRatio(source.devicePixelRatioF());
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.drawPixmap(0, 0, source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(result.rect(), color);
    painter.end();
    return result;
}
