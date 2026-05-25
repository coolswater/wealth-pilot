#include "SvgIconWidget.h"
#include "SvgIconEngine.h"
#include "src/ui/ThemeManager.h"
#include "core/config/Tokens.h"

QIcon SvgIconWidget::icon(const QString& path, const QColor& color)
{
    return QIcon(new SvgIconEngine(path, color));
}
QIcon SvgIconWidget::themedIcon(const QString& path, const QString& colorRole)
{
    QColor color;
    if (colorRole == QStringLiteral("primary"))
    {
        color = QColor(Tokens::Colors::Primary);
    }
    else if (colorRole == QStringLiteral("danger"))
    {
        color = QColor(Tokens::Colors::Danger);
    }
    else if (colorRole == QStringLiteral("success"))
    {
        color = QColor(Tokens::Colors::Success);
    }
    else if (colorRole == QStringLiteral("warning"))
    {
        color = QColor(Tokens::Colors::Warning);
    }
    else
    {
        color = QColor(Tokens::Colors::TextPrimary);
    }
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

