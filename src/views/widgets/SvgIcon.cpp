#include "SvgIcon.h"
QIcon SvgIcon::icon(const QString& path, const QColor& color)
{
    return QIcon(new SvgIconEngine(path, color));
}
QIcon SvgIcon::themedIcon(const QString& path, const QString& colorRole)
{
    QColor color = ThemeManager::instance()->color(colorRole);
    return QIcon(new SvgIconEngine(path, color));
}
QIcon SvgIcon::riseIcon(const QString& path)
{
    return QIcon(new SvgIconEngine(path, ThemeManager::instance()->riseColor()));
}
QIcon SvgIcon::fallIcon(const QString& path)
{
    return QIcon(new SvgIconEngine(path, ThemeManager::instance()->fallColor()));
}
QList<QIcon> SvgIcon::batchCreate(const QStringList& paths, const QSize& size)
{
    QList<QIcon> icons;
    icons.reserve(paths.size());
    for (const QString& path : paths) {
        SvgIconEngine* engine = new SvgIconEngine(path);
        if (size.isValid()) {
            engine->preRender(size);
        }
        icons.append(QIcon(engine));
    }
    return icons;
}
void SvgIcon::clearCache()
{
    SvgIconEngine::clearCache();
}
