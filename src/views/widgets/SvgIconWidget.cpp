#include "SvgIconWidget.h"
#include "SvgIconEngine.h"
#include "src/core/ThemeManager.h"

QIcon SvgIconWidget::icon(const QString& path, const QColor& color)
{
    return QIcon(new SvgIconEngine(path, color));
}
QIcon SvgIconWidget::themedIcon(const QString& path, const QString& colorRole)
{
    QColor color = ThemeManager::instance()->color(colorRole);
    return QIcon(new SvgIconEngine(path, color));
}
QList<QIcon> SvgIconWidget::batchCreate(const QStringList& paths, const QSize& size)
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
void SvgIconWidget::clearCache()
{
    SvgIconEngine::clearCache();
}
