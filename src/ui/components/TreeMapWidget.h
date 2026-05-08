#ifndef TREEMAPWIDGET_H
#define TREEMAPWIDGET_H

#include <QWidget>
#include <QVector>
#include <QMap>
#include <QTimer>
#include <QPainter>

#include "models/StockQuoteItem.h"
#include "ui/ThemeManager.h"

/**
 * @brief 板块（行业）结构
 * 包含板块内的所有股票和板块级信息
 */
struct IndustryBlock {
    QString name;                    // 板块名称（如"银行"、"半导体"）
    QColor themeColor;               // 板块主题色（用于边框和标题）
    QVector<StockQuoteItem*> items;  // 板块内的股票指针列表
    double totalMarketCap;           // 板块总市值
    QRectF rect;                     // 板块在画布上的区域
    bool expanded;                   // 是否展开（预留功能）

    IndustryBlock() : totalMarketCap(0), expanded(true) {}
};

class TreeMapWidget : public QWidget
{
    Q_OBJECT

public:
    enum LayoutMode {
        Unified,        // 统一布局（所有股票混在一起，按市值）
        ByIndustry      // 按板块分类布局（类似52etf.site）
    };

    explicit TreeMapWidget(QWidget *parent = nullptr);
    ~TreeMapWidget() override;

    // 数据操作
    void setData(const QVector<StockQuoteItem> &items);
    void clearData();

    // 视图控制
    void setLayoutMode(LayoutMode mode);  // 切换布局模式
    void setColorScheme(int scheme);      // 0:红涨绿跌, 1:绿涨红跌

    // 筛选
    void filterByMarket(const QString &market);
    void filterByIndustry(const QString &industry);
    void search(const QString &keyword);

    // 选中操作
    [[nodiscard]] StockQuoteItem* selectedItem() const;
    void selectItem(const QString &code);

    // 复盘模式
    void setReviewMode(bool enabled);
    void nextTimeFrame();
    void prevTimeFrame();

    // 导航（公共接口，供DashboardPage调用）
    void navigateNext();      // 选择下一个
    void navigatePrevious();  // 选择上一个
    void clearSelection();    // 清除选择

signals:
    void itemClicked(const StockQuoteItem &item);
    void itemDoubleClicked(const StockQuoteItem &item);
    void itemHovered(const StockQuoteItem &item);
    void blockClicked(const QString &industryName);  // 点击板块信号
    void statsChanged(int up, int flat, int down, double totalTurnover);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    // 布局算法
    void calculateLayout();
    void calculateUnifiedLayout();      // 统一布局（原算法）
    void calculateIndustryLayout();     // 【新增】按板块分类布局

    // Squarified算法（用于板块内部）
    void squarifyItems(QVector<StockQuoteItem*> &items, const QRectF &rect);
    void layoutRow(QVector<StockQuoteItem*> &items, int start, int count,
                   const QRectF &rect, double totalWeight, bool vertical);

    // 绘制
    void drawBackground(QPainter *p);
    void drawIndustryBlocks(QPainter *p);   // 【新增】绘制板块背景
    void drawBlockLabels(QPainter *p);      // 【新增】绘制板块标题
    void drawTiles(QPainter *p);            // 绘制个股色块
    void drawBorders(QPainter *p);
    void drawLabels(QPainter *p);
    void drawTooltip(QPainter *p);
    void drawLegend(QPainter *p);           // 【新增】绘制图例

    // 辅助
    void groupByIndustry();              // 【新增】按行业分组
    QColor generateIndustryColor(int index);  // 【新增】生成板块颜色
    StockQuoteItem* itemAt(const QPoint &pos);
    IndustryBlock* blockAt(const QPoint &pos);  // 【新增】获取点击的板块
    QString formatMarketCap(double cap);
    QColor interpolateColor(double percent);
    void updateStats();
    void selectNextItem(int direction);
    void loadHistoryFrame(int frameIndex);

    // 成员变量
    QVector<StockQuoteItem> m_allItems;
    QVector<StockQuoteItem*> m_filteredItems;
    QVector<StockQuoteItem*> m_visibleItems;
    QMap<QString, IndustryBlock> m_industryBlocks;  // 【新增】板块映射

    LayoutMode m_layoutMode;
    int m_colorScheme;

    StockQuoteItem *m_selectedItem;
    StockQuoteItem *m_hoveredItem;
    IndustryBlock *m_hoveredBlock;          // 【新增】悬停的板块
    QPoint m_mousePos;

    bool m_reviewMode;
    int m_currentTimeIndex;
    QVector<QMap<QString, StockQuoteItem>> m_historyData;

    QTimer *m_animTimer{};
    double m_animProgress;

    QRectF m_viewRect;
    double m_minMarketCap;
    int m_selectedIndex;                    // 当前选中项的索引（用于键盘导航）
};

#endif