/**
 * @file StyleMigrationExamples.cpp
 * @brief 样式迁移示例代码
 * 
 * 本文件展示如何从 PageStyles 迁移到 QSS 样式系统
 */

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QGroupBox>
#include "ui/ThemeManager.h"

/**
 * @brief 样式迁移示例类
 */
class StyleMigrationExamples
{
public:
    // ========================================================================
    // 1. 按钮样式迁移
    // ========================================================================
    
    /**
     * @brief 迁移前：使用 PageStyles
     */
    void buttonStyleOld()
    {
        QPushButton* primaryBtn = new QPushButton();
        // 旧方式：调用 PageStyles 方法
        primaryBtn->setStyleSheet(PageStyles::primaryButton());
        
        QPushButton* secondaryBtn = new QPushButton();
        secondaryBtn->setStyleSheet(PageStyles::secondaryButton());
    }
    
    /**
     * @brief 迁移后：使用属性选择器
     */
    void buttonStyleNew()
    {
        QPushButton* primaryBtn = new QPushButton();
        // 新方式：设置属性，让全局样式生效
        primaryBtn->setProperty("primary", true);
        // 刷新样式
        primaryBtn->style()->unpolish(primaryBtn);
        primaryBtn->style()->polish(primaryBtn);
        
        QPushButton* secondaryBtn = new QPushButton();
        secondaryBtn->setProperty("secondary", true);
        secondaryBtn->style()->unpolish(secondaryBtn);
        secondaryBtn->style()->polish(secondaryBtn);
        
        // 危险按钮
        QPushButton* dangerBtn = new QPushButton();
        dangerBtn->setProperty("danger", true);
        dangerBtn->style()->unpolish(dangerBtn);
        dangerBtn->style()->polish(dangerBtn);
        
        // 成功按钮
        QPushButton* successBtn = new QPushButton();
        successBtn->setProperty("success", true);
        successBtn->style()->unpolish(successBtn);
        successBtn->style()->polish(successBtn);
        
        // 图标按钮
        QPushButton* iconBtn = new QPushButton();
        iconBtn->setProperty("icon", true);
        iconBtn->style()->unpolish(iconBtn);
        iconBtn->style()->polish(iconBtn);
    }
    
    // ========================================================================
    // 2. 涨跌颜色迁移
    // ========================================================================
    
    /**
     * @brief 迁移前：硬编码颜色
     */
    void trendColorOld(double change)
    {
        QLabel* priceLabel = new QLabel();
        
        if (change > 0) {
            // 旧方式：硬编码颜色
            priceLabel->setStyleSheet("color: #EF4444;");  // 红色
        } else if (change < 0) {
            priceLabel->setStyleSheet("color: #10B981;");  // 绿色
        } else {
            priceLabel->setStyleSheet("color: #9CA3AF;");  // 灰色
        }
    }
    
    /**
     * @brief 迁移后：使用属性选择器
     */
    void trendColorNew(double change)
    {
        QLabel* priceLabel = new QLabel();
        
        // 新方式：使用属性选择器
        if (change > 0) {
            priceLabel->setProperty("trend", "up");
        } else if (change < 0) {
            priceLabel->setProperty("trend", "down");
        } else {
            priceLabel->setProperty("trend", "flat");
        }
        
        // 刷新样式
        priceLabel->style()->unpolish(priceLabel);
        priceLabel->style()->polish(priceLabel);
    }
    
    /**
     * @brief 迁移后：使用 ThemeManager 获取颜色（需要动态颜色时）
     */
    void trendColorWithThemeManager(double change)
    {
        QLabel* priceLabel = new QLabel();
        
        // 获取当前主题颜色
        ThemeColors theme = ThemeManager::instance()->currentTheme();
        
        QString color;
        if (change > 0) {
            color = theme.danger;  // 上涨用红色
        } else if (change < 0) {
            color = theme.success; // 下跌用绿色
        } else {
            color = theme.textSecondary;
        }
        
        priceLabel->setStyleSheet(QString("color: %1;").arg(color));
    }
    
    // ========================================================================
    // 3. 输入框样式迁移
    // ========================================================================
    
    /**
     * @brief 迁移前：使用 PageStyles
     */
    void inputFieldOld()
    {
        QLineEdit* lineEdit = new QLineEdit();
        // 旧方式
        lineEdit->setStyleSheet(PageStyles::inputField());
        
        QComboBox* comboBox = new QComboBox();
        comboBox->setStyleSheet(PageStyles::comboBox());
    }
    
    /**
     * @brief 迁移后：使用全局样式
     */
    void inputFieldNew()
    {
        QLineEdit* lineEdit = new QLineEdit();
        // 新方式：全局样式自动生效，无需设置
        
        QComboBox* comboBox = new QComboBox();
        // 全局样式自动生效
    }
    
    // ========================================================================
    // 4. 表格样式迁移
    // ========================================================================
    
    /**
     * @brief 迁移前：使用 PageStyles
     */
    void tableOld()
    {
        QTableWidget* table = new QTableWidget();
        // 旧方式
        table->setStyleSheet(PageStyles::table());
    }
    
    /**
     * @brief 迁移后：使用全局样式
     */
    void tableNew()
    {
        QTableWidget* table = new QTableWidget();
        // 新方式：全局样式自动生效
    }
    
    // ========================================================================
    // 5. 分组框样式迁移
    // ========================================================================
    
    /**
     * @brief 迁移前：使用 PageStyles
     */
    void groupBoxOld()
    {
        QGroupBox* groupBox = new QGroupBox("分组标题");
        // 旧方式
        groupBox->setStyleSheet(PageStyles::groupBox());
    }
    
    /**
     * @brief 迁移后：使用全局样式
     */
    void groupBoxNew()
    {
        QGroupBox* groupBox = new QGroupBox("分组标题");
        // 新方式：全局样式自动生效
    }
    
    // ========================================================================
    // 6. 卡片样式迁移
    // ========================================================================
    
    /**
     * @brief 迁移前：使用 PageStyles
     */
    void cardOld()
    {
        QWidget* card = new QWidget();
        // 旧方式
        card->setStyleSheet(PageStyles::cardContainer());
        
        // 带强调色的卡片
        QWidget* accentCard = new QWidget();
        accentCard->setStyleSheet(PageStyles::statCard("#3B82F6"));
    }
    
    /**
     * @brief 迁移后：使用对象名或属性
     */
    void cardNew()
    {
        // 普通卡片
        QWidget* card = new QWidget();
        card->setObjectName("cardWidget");
        // QSS: #cardWidget { ... }
        
        // 带主题的卡片
        QWidget* accentCard = new QWidget();
        accentCard->setProperty("theme", "primary");
        accentCard->style()->unpolish(accentCard);
        accentCard->style()->polish(accentCard);
        
        // 成功主题卡片
        QWidget* successCard = new QWidget();
        successCard->setProperty("theme", "success");
        successCard->style()->unpolish(successCard);
        successCard->style()->polish(successCard);
    }
    
    // ========================================================================
    // 7. 文本样式迁移
    // ========================================================================
    
    /**
     * @brief 迁移前：使用 PageStyles
     */
    void textOld()
    {
        QLabel* titleLabel = new QLabel("标题");
        titleLabel->setStyleSheet(PageStyles::titleText());
        
        QLabel* subtitleLabel = new QLabel("副标题");
        subtitleLabel->setStyleSheet(PageStyles::subtitleText());
        
        QLabel* valueLabel = new QLabel("123.45");
        valueLabel->setStyleSheet(PageStyles::valueText());
        
        QLabel* label = new QLabel("标签");
        label->setStyleSheet(PageStyles::labelText());
    }
    
    /**
     * @brief 迁移后：使用对象名选择器
     */
    void textNew()
    {
        QLabel* titleLabel = new QLabel("标题");
        titleLabel->setObjectName("titleLabel");
        // QSS: QLabel#titleLabel { font-size: 20px; font-weight: bold; }
        
        QLabel* subtitleLabel = new QLabel("副标题");
        subtitleLabel->setObjectName("subtitleLabel");
        // QSS: QLabel#subtitleLabel { font-size: 14px; color: ${textSecondary}; }
        
        QLabel* valueLabel = new QLabel("123.45");
        valueLabel->setObjectName("valueLabel");
        // QSS: QLabel#valueLabel { font-size: 18px; font-weight: bold; }
        
        QLabel* label = new QLabel("标签");
        label->setObjectName("labelText");
        // QSS: QLabel#labelText { font-size: 12px; color: ${textSecondary}; }
    }
    
    // ========================================================================
    // 8. 批量更新样式
    // ========================================================================
    
    /**
     * @brief 批量更新多个控件的样式
     */
    void batchUpdateStyle(const QList<QWidget*>& widgets, const QString& property, const QString& value)
    {
        // 批量设置属性
        for (QWidget* widget : widgets) {
            widget->setProperty(property.toUtf8().constData(), value);
        }
        
        // 统一刷新（性能更好）
        if (!widgets.isEmpty() && widgets.first()->parentWidget()) {
            widgets.first()->parentWidget()->update();
        }
    }
    
    /**
     * @brief 示例：批量更新涨跌颜色
     */
    void updateTrendColors(const QHash<QLabel*, double>& priceLabels)
    {
        for (auto it = priceLabels.begin(); it != priceLabels.end(); ++it) {
            QLabel* label = it.key();
            double change = it.value();
            
            if (change > 0) {
                label->setProperty("trend", "up");
            } else if (change < 0) {
                label->setProperty("trend", "down");
            } else {
                label->setProperty("trend", "flat");
            }
        }
        
        // 统一刷新父容器
        if (!priceLabels.isEmpty()) {
            QLabel* firstLabel = priceLabels.begin().key();
            if (firstLabel->parentWidget()) {
                firstLabel->parentWidget()->update();
            }
        }
    }
    
    // ========================================================================
    // 9. 主题切换响应
    // ========================================================================
    
    /**
     * @brief 监听主题切换
     */
    void setupThemeChangeListener(QWidget* widget)
    {
        // 注册主题变化监听器
        ThemeManager::instance()->registerThemeChangeListener(widget, [widget]() {
            // 主题变化时刷新控件
            widget->update();
        });
        
        // 或者连接信号
        QObject::connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            [widget](ThemeType type) {
                // 主题变化时的处理
                widget->update();
            });
    }
    
    // ========================================================================
    // 10. 辅助函数
    // ========================================================================
    
    /**
     * @brief 刷新控件样式
     */
    static void refreshStyle(QWidget* widget)
    {
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
    }
    
    /**
     * @brief 批量刷新控件样式
     */
    static void refreshStyles(const QList<QWidget*>& widgets)
    {
        for (QWidget* widget : widgets) {
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
        }
    }
    
    /**
     * @brief 设置属性并刷新样式
     */
    static void setPropertyAndRefresh(QWidget* widget, const QString& property, const QVariant& value)
    {
        widget->setProperty(property.toUtf8().constData(), value);
        refreshStyle(widget);
    }
};

// ============================================================================
// 使用示例
// ============================================================================

/**
 * @brief 在页面中使用新样式系统
 */
class ExamplePage : public QWidget
{
public:
    ExamplePage(QWidget* parent = nullptr) : QWidget(parent)
    {
        setupUI();
    }
    
private:
    void setupUI()
    {
        // 创建按钮
        QPushButton* saveBtn = new QPushButton("保存", this);
        StyleMigrationExamples::setPropertyAndRefresh(saveBtn, "primary", true);
        
        QPushButton* cancelBtn = new QPushButton("取消", this);
        StyleMigrationExamples::setPropertyAndRefresh(cancelBtn, "secondary", true);
        
        // 创建价格标签
        QLabel* priceLabel = new QLabel("+12.5%", this);
        StyleMigrationExamples::setPropertyAndRefresh(priceLabel, "trend", "up");
        
        // 监听主题变化
        ThemeManager::instance()->registerThemeChangeListener(this, [this]() {
            this->update();
        });
    }
};
