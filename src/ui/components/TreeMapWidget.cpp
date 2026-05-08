#include "TreeMapWidget.h"
#include "core/config/Tokens.h"
#include "ui/ThemeManager.h"
#include <QPainter>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QToolTip>
#include <QtMath>
#include <QDebug>

/**
 * @brief 构造函数
 * 初始化热力图控件，设置默认参数和UI属性
 * @param parent 父窗口指针
 */
TreeMapWidget::TreeMapWidget(QWidget* parent)
    : QWidget(parent)
      , m_layoutMode(ByIndustry) // 默认使用板块分类模式（类似52etf.site）
      , m_colorScheme(0) // 0=红涨绿跌(A股风格)
      , m_selectedItem(nullptr) // 当前无选中项
      , m_hoveredItem(nullptr) // 当前无悬停项
      , m_hoveredBlock(nullptr) // 当前无悬停板块
      , m_reviewMode(false) // 复盘模式默认关闭
      , m_currentTimeIndex(0) // 复盘时间轴索引
      , m_animProgress(1.0) // 动画进度(暂未使用)
      , m_minMarketCap(0) // 最小显示市值(0表示不过滤)
      , m_selectedIndex(-1) // 键盘导航当前索引
{
    // 启用鼠标跟踪，确保mouseMoveEvent实时触发（不需要按住鼠标）
    setMouseTracking(true);

    // 设置强焦点策略，确保能接收键盘事件
    setFocusPolicy(Qt::StrongFocus);

    // 使用主题管理器设置背景色
    setStyleSheet(QString("background-color: %1;").arg(Tokens::Colors::BgBase));
}

/**
 * @brief 析构函数
 * 使用默认实现，Qt的父子对象机制会自动清理子对象
 */
TreeMapWidget::~TreeMapWidget() = default;

/**
 * @brief 设置股票数据（主入口）
 * 接收完整股票列表，建立索引，并触发重绘
 * @param items 股票数据列表（传值拷贝，内部维护副本）
 */
void TreeMapWidget::setData(const QVector<StockQuoteItem>& items)
{
    // 保存数据副本
    m_allItems = items;

    // 建立指针索引（后续操作使用指针，避免拷贝）
    m_filteredItems.clear();
    for (int i = 0; i < m_allItems.size(); ++i)
    {
        m_filteredItems.append(&m_allItems[i]);
    }

    // 重置选中状态
    m_selectedItem = nullptr;
    m_hoveredItem = nullptr;
    m_hoveredBlock = nullptr;
    m_selectedIndex = -1;

    // 如果是板块模式，按行业分组
    if (m_layoutMode == ByIndustry)
    {
        groupByIndustry();
    }

    // 计算布局并刷新显示
    calculateLayout();
    update();
    updateStats(); // 更新统计信息（涨跌家数等）
}

/**
 * @brief 清空所有数据
 * 重置所有容器和状态，UI显示空白
 */
void TreeMapWidget::clearData()
{
    m_allItems.clear();
    m_filteredItems.clear();
    m_visibleItems.clear();
    m_industryBlocks.clear();
    m_selectedItem = nullptr;
    m_hoveredItem = nullptr;
    m_hoveredBlock = nullptr;
    update(); // 触发重绘，显示"暂无数据"
}

/**
 * @brief 设置布局模式
 * 切换统一布局或板块分类布局
 * @param mode 布局模式枚举值
 */
void TreeMapWidget::setLayoutMode(LayoutMode mode)
{
    if (m_layoutMode != mode)
    {
        m_layoutMode = mode;

        // 切换到板块模式时需要重新分组
        if (mode == ByIndustry)
        {
            groupByIndustry();
        }

        calculateLayout();
        update();
    }
}

/**
 * @brief 设置颜色方案
 * @param scheme 0=红涨绿跌(A股)，1=绿涨红跌(国际惯例)
 */
void TreeMapWidget::setColorScheme(int scheme)
{
    m_colorScheme = scheme;
    update(); // 颜色变化只需重绘，不需要重新计算布局
}

/**
 * @brief 按市场筛选
 * 根据市场代码筛选显示股票（上证/深证/创业板等）
 * @param market 市场标识符："all"-全部，"sh"-上证，"sz"-深证，"cyb"-创业板等
 */
void TreeMapWidget::filterByMarket(const QString& market)
{
    // 重置为全部数据
    if (market == "all" || market.isEmpty())
    {
        m_filteredItems.clear();
        for (int i = 0; i < m_allItems.size(); ++i)
        {
            m_filteredItems.append(&m_allItems[i]);
        }
    }
    else
    {
        // 根据代码前缀筛选不同市场
        m_filteredItems.clear();
        for (int i = 0; i < m_allItems.size(); ++i)
        {
            QString code = m_allItems[i].code.toLower();

            // 上证主板：6开头
            if (market == "sh" && code.startsWith("6"))
            {
                m_filteredItems.append(&m_allItems[i]);
            }
            // 深证主板：0开头，创业板：3开头
            else if (market == "sz" && (code.startsWith("0") || code.startsWith("3")))
            {
                m_filteredItems.append(&m_allItems[i]);
            }
            // 创业板
            else if (market == "cyb" && code.startsWith("3"))
            {
                m_filteredItems.append(&m_allItems[i]);
            }
            // 科创板：68开头
            else if (market == "kcb" && code.startsWith("68"))
            {
                m_filteredItems.append(&m_allItems[i]);
            }
            // 期货：isFutures标志
            else if (market == "futures" && m_allItems[i].isFutures)
            {
                m_filteredItems.append(&m_allItems[i]);
            }
        }
    }

    // 板块模式下需要重新分组
    if (m_layoutMode == ByIndustry)
    {
        groupByIndustry();
    }

    calculateLayout();
    update();
    updateStats();
}

/**
 * @brief 按行业筛选
 * 只显示指定行业的股票
 * @param industry 行业名称，如"银行"、"半导体"等
 */
void TreeMapWidget::filterByIndustry(const QString& industry)
{
    if (industry == "全部" || industry.isEmpty())
    {
        filterByMarket("all");
        return;
    }

    m_filteredItems.clear();
    for (int i = 0; i < m_allItems.size(); ++i)
    {
        if (m_allItems[i].industry == industry)
        {
            m_filteredItems.append(&m_allItems[i]);
        }
    }

    if (m_layoutMode == ByIndustry)
    {
        groupByIndustry();
    }

    calculateLayout();
    update();
    updateStats();
}

/**
 * @brief 搜索功能
 * 根据代码或名称模糊匹配，实时过滤显示
 * @param keyword 搜索关键词，支持代码（如"600001"）或名称（如"平安"）
 */
void TreeMapWidget::search(const QString& keyword)
{
    // 空关键词恢复全部显示
    if (keyword.isEmpty())
    {
        filterByMarket("all");
        return;
    }

    QString lowerKey = keyword.toLower();
    m_filteredItems.clear();

    // 遍历所有原始数据（不是已筛选的），进行模糊匹配
    for (int i = 0; i < m_allItems.size(); ++i)
    {
        // 匹配名称或代码（不区分大小写）
        if (m_allItems[i].name.contains(keyword) ||
            m_allItems[i].code.toLower().contains(lowerKey))
        {
            m_filteredItems.append(&m_allItems[i]);
        }
    }

    if (m_layoutMode == ByIndustry)
    {
        groupByIndustry();
    }

    calculateLayout();
    update();
    updateStats();
}

/**
 * @brief 获取当前选中的股票
 * @return 选中项的指针，无选中返回nullptr
 */
StockQuoteItem* TreeMapWidget::selectedItem() const
{
    return m_selectedItem;
}

/**
 * @brief 根据代码选中指定股票
 * @param code 股票代码（如"000001"）
 */
void TreeMapWidget::selectItem(const QString& code)
{
    for (auto item : m_filteredItems)
    {
        if (item->code == code)
        {
            m_selectedItem = item;
            // 更新键盘导航索引，确保后续导航连贯
            m_selectedIndex = m_visibleItems.indexOf(item);
            update();
            emit itemClicked(*item);
            return;
        }
    }
    // 未找到则清空选择
    m_selectedItem = nullptr;
    m_selectedIndex = -1;
    update();
}

/**
 * @brief 清空当前选中
 */
void TreeMapWidget::clearSelection()
{
    m_selectedItem = nullptr;
    m_selectedIndex = -1;
    update();
}

/**
 * @brief 设置复盘模式
 * @param enabled true开启复盘，false关闭
 */
void TreeMapWidget::setReviewMode(bool enabled)
{
    m_reviewMode = enabled;
    if (enabled && !m_historyData.isEmpty())
    {
        m_currentTimeIndex = 0;
        loadHistoryFrame(0);
    }
}

/**
 * @brief 复盘：前进到下一时间帧
 */
void TreeMapWidget::nextTimeFrame()
{
    if (!m_reviewMode || m_historyData.isEmpty()) return;

    if (m_currentTimeIndex < m_historyData.size() - 1)
    {
        m_currentTimeIndex++;
        loadHistoryFrame(m_currentTimeIndex);
    }
}

/**
 * @brief 复盘：后退到上一时间帧
 */
void TreeMapWidget::prevTimeFrame()
{
    if (!m_reviewMode || m_historyData.isEmpty()) return;

    if (m_currentTimeIndex > 0)
    {
        m_currentTimeIndex--;
        loadHistoryFrame(m_currentTimeIndex);
    }
}

/**
 * @brief 键盘导航：选择下一个项目
 * 循环导航，到达末尾后回到开头
 */
void TreeMapWidget::navigateNext()
{
    if (m_visibleItems.isEmpty()) return;

    m_selectedIndex++;
    if (m_selectedIndex >= m_visibleItems.size())
    {
        m_selectedIndex = 0; // 循环到开头
    }

    m_selectedItem = m_visibleItems[m_selectedIndex];
    emit itemClicked(*m_selectedItem);
    update();
}

/**
 * @brief 键盘导航：选择上一个项目
 */
void TreeMapWidget::navigatePrevious()
{
    if (m_visibleItems.isEmpty()) return;

    m_selectedIndex--;
    if (m_selectedIndex < 0)
    {
        m_selectedIndex = m_visibleItems.size() - 1; // 循环到末尾
    }

    m_selectedItem = m_visibleItems[m_selectedIndex];
    emit itemClicked(*m_selectedItem);
    update();
}

/**
 * @brief 【核心】按行业分组
 * 将filteredItems中的股票按industry字段分组，创建板块结构
 * 每个板块计算总市值，分配主题色
 */
void TreeMapWidget::groupByIndustry()
{
    m_industryBlocks.clear();

    // 遍历所有筛选后的股票，按行业归类
    for (auto item : m_filteredItems)
    {
        // 如果行业为空，归入"其他"
        QString industry = item->industry.isEmpty() ? "其他" : item->industry;

        // 如果是期货，按品种类型分组（或归入"期货"）
        if (item->isFutures && industry == "其他")
        {
            industry = "期货";
        }

        // 初始化新板块
        if (!m_industryBlocks.contains(industry))
        {
            IndustryBlock block;
            block.name = industry;
            block.themeColor = generateIndustryColor(m_industryBlocks.size());
            m_industryBlocks[industry] = block;
        }

        // 添加到对应板块，累加市值
        m_industryBlocks[industry].items.append(item);
        m_industryBlocks[industry].totalMarketCap += item->marketCap;
    }
}

/**
 * @brief 生成板块主题色
 * 使用预设的20个低饱和度颜色，确保视觉区分度且不过于鲜艳
 * @param index 板块索引，循环使用颜色表
 * @return 板块边框和标题的背景色
 */
QColor TreeMapWidget::generateIndustryColor(int index)
{
    // 预设颜色：低饱和度，适合作为边框和半透明背景
    static const QColor colors[] = {
        QColor(80, 120, 160), // 蓝灰 - 科技/电子
        QColor(120, 80, 160), // 紫灰 - 医药
        QColor(160, 80, 80), // 红灰 - 能源
        QColor(80, 160, 120), // 绿灰 - 环保/农业
        QColor(160, 120, 80), // 橙灰 - 消费
        QColor(80, 80, 120), // 深蓝灰 - 金融
        QColor(120, 160, 80), // 黄绿灰 - 建筑
        QColor(160, 80, 120), // 玫红灰 - 传媒
        QColor(80, 160, 160), // 青灰 - 通信
        QColor(140, 100, 100), // 棕灰 - 原材料
        QColor(100, 140, 100), // 深绿灰 - 交通运输
        QColor(100, 100, 140), // 靛蓝灰 - 国防
        QColor(140, 140, 100), // 橄榄灰 - 公用事业
        QColor(140, 100, 140), // 紫红灰 - 综合
        QColor(100, 140, 140), // 青蓝灰 - 房地产
        QColor(120, 120, 120), // 灰 - 其他
        QColor(160, 140, 120), // 米灰 - 轻工
        QColor(140, 160, 120), // 草绿灰 - 纺织
        QColor(120, 140, 160), // 天蓝灰 - 汽车
        QColor(160, 120, 140) // 粉灰 - 商贸
    };
    return colors[index % 20];
}

/**
 * @brief 计算布局（调度函数）
 * 根据当前布局模式选择对应的布局算法
 */
void TreeMapWidget::calculateLayout()
{
    if (m_filteredItems.isEmpty()) return;

    // 留5像素边距
    m_viewRect = rect().adjusted(5, 5, -5, -5);

    // 根据模式选择布局算法
    if (m_layoutMode == ByIndustry && !m_industryBlocks.isEmpty())
    {
        calculateIndustryLayout(); // 板块分类布局（类似52etf.site）
    }
    else
    {
        calculateUnifiedLayout(); // 统一混合布局
    }
}

/**
 * @brief 【核心算法】板块分类布局
 * 模拟52etf.site的实现：
 * 1. 外层：使用Squarified算法排列板块（大板块占大块区域）
 * 2. 内层：每个板块内部再用Squarified算法排列个股
 *
 * 这样实现"一眼看出哪个板块大、哪个板块涨"的效果
 */
void TreeMapWidget::calculateIndustryLayout()
{
    if (m_industryBlocks.isEmpty()) return;

    // 将板块转为指针列表，便于排序和遍历
    QVector<IndustryBlock*> blocks;
    double totalCap = 0;

    for (auto& block : m_industryBlocks)
    {
        blocks.append(&block);
        totalCap += block.totalMarketCap;
    }

    // 按总市值降序排序：大板块优先获得好位置
    std::sort(blocks.begin(), blocks.end(),
              [](IndustryBlock* a, IndustryBlock* b)
              {
                  return a->totalMarketCap > b->totalMarketCap;
              });

    // 使用Squarified算法进行板块级布局
    QRectF remaining = m_viewRect;
    double remainingWeight = totalCap;
    bool vertical = true; // 起始切分方向：true=垂直（从左到右），false=水平（从上到下）

    int start = 0;
    while (start < blocks.size())
    {
        // ---- 步骤1：确定这一行包含哪些板块（优化长宽比） ----
        double rowWeight = 0;
        double minAspectRatio = 1e10; // 当前最小长宽比（越接近1越接近正方形）
        int split = start;

        for (int i = start; i < blocks.size(); i++)
        {
            rowWeight += blocks[i]->totalMarketCap;

            // 计算这一行的总面积
            double rowArea = remaining.width() * remaining.height() * (rowWeight / remainingWeight);
            double side = vertical ? remaining.width() : remaining.height(); // 切分边
            double otherSide = rowArea / side; // 另一边

            // 计算这一行内最大长宽比
            double maxAspect = std::max(side / otherSide, otherSide / side);

            // 如果长宽比改善了，继续添加板块到这一行
            if (maxAspect < minAspectRatio)
            {
                minAspectRatio = maxAspect;
                split = i;
            }
            else
            {
                // 长宽比开始恶化，停止这一行
                break;
            }
        }

        // ---- 步骤2：布局这一行的板块 ----
        double rowWeightSum = 0;
        for (int i = start; i <= split; i++)
        {
            rowWeightSum += blocks[i]->totalMarketCap;
        }

        double rowArea = remaining.width() * remaining.height() * (rowWeightSum / remainingWeight);
        double rowSide = vertical ? remaining.width() : remaining.height();
        double rowOtherSide = rowArea / rowSide;

        // 在这一行内按比例分配位置
        double offset = 0;
        for (int i = start; i <= split; i++)
        {
            double ratio = blocks[i]->totalMarketCap / rowWeightSum;
            double size = rowSide * ratio;

            // 设置板块的外接矩形
            if (vertical)
            {
                blocks[i]->rect = QRectF(remaining.x() + offset, remaining.y(), size, rowOtherSide);
            }
            else
            {
                blocks[i]->rect = QRectF(remaining.x(), remaining.y() + offset, rowOtherSide, size);
            }
            offset += size;

            // ---- 步骤3：在板块内部布局个股 ----
            if (!blocks[i]->items.isEmpty())
            {
                // 顶部留出22像素给标题栏（行业名+统计）
                QRectF innerRect = blocks[i]->rect.adjusted(2, 22, -2, -2);
                squarifyItems(blocks[i]->items, innerRect);
            }
        }

        // ---- 步骤4：更新剩余区域，继续下一行 ----
        if (vertical)
        {
            remaining = QRectF(remaining.x(), remaining.y() + rowOtherSide,
                               remaining.width(), remaining.height() - rowOtherSide);
        }
        else
        {
            remaining = QRectF(remaining.x() + rowOtherSide, remaining.y(),
                               remaining.width() - rowOtherSide, remaining.height());
        }

        remainingWeight -= rowWeightSum;
        vertical = !vertical; // 交替切分方向（类似棋盘）
        start = split + 1;
    }

    // 收集所有可见个股到统一列表（用于后续遍历和命中检测）
    m_visibleItems.clear();
    for (auto block : blocks)
    {
        for (auto item : block->items)
        {
            if (item->rect.isValid()) m_visibleItems.append(item);
        }
    }
}

/**
 * @brief 统一布局（非板块模式）
 * 所有股票混在一起，单纯按市值大小排列
 */
void TreeMapWidget::calculateUnifiedLayout()
{
    if (m_filteredItems.isEmpty()) return;
    squarifyItems(m_filteredItems, m_viewRect);
    m_visibleItems = m_filteredItems;
}

/**
 * @brief 【核心算法】Squarified矩形树图布局
 * 经典算法，确保所有矩形尽可能接近正方形（长宽比接近1:1）
 * 避免出现过细或过扁的矩形，提高可视性和可读性
 *
 * @param items 要布局的股票列表（已按市值降序）
 * @param rect 可用矩形区域
 */
void TreeMapWidget::squarifyItems(QVector<StockQuoteItem*>& items, const QRectF& rect)
{
    if (items.isEmpty() || !rect.isValid()) return;

    // 按市值降序排列（Squarified算法要求）
    std::sort(items.begin(), items.end(),
              [](StockQuoteItem* a, StockQuoteItem* b)
              {
                  return a->marketCap > b->marketCap;
              });

    // 计算总权重（总市值）
    double totalWeight = 0;
    for (auto item : items)
    {
        if (item->marketCap >= m_minMarketCap) totalWeight += item->marketCap;
    }

    if (totalWeight <= 0) return;

    // 初始化布局区域
    QRectF remaining = rect;
    double remainingWeight = totalWeight;
    // 选择起始切分方向：横向较长则垂直切分（从左到右），否则水平切分
    bool vertical = remaining.width() > remaining.height();

    int start = 0;
    while (start < items.size())
    {
        // ---- 寻找最优行组成 ----
        double rowWeight = 0;
        double minAspect = 1e10;
        int split = start;

        // 尝试向当前行添加股票，直到长宽比开始恶化
        for (int i = start; i < items.size(); i++)
        {
            if (items[i]->marketCap < m_minMarketCap) continue; // 跳过太小的（过滤功能）

            rowWeight += items[i]->marketCap;
            double rowArea = remaining.width() * remaining.height() * (rowWeight / remainingWeight);
            double side = vertical ? remaining.width() : remaining.height();
            double otherSide = rowArea / side;

            // 计算当前行内所有股票的长宽比，找最差（最大）的
            double maxAspect = 0;
            for (int j = start; j <= i; j++)
            {
                if (items[j]->marketCap < m_minMarketCap) continue;
                double itemSize = side * (items[j]->marketCap / rowWeight);
                double aspect = std::max(otherSide / itemSize, itemSize / otherSide);
                maxAspect = std::max(maxAspect, aspect);
            }

            // 如果比之前的行更好（更接近正方形），继续添加
            if (maxAspect < minAspect)
            {
                minAspect = maxAspect;
                split = i;
            }
        }

        // ---- 布局这一行 ----
        double rowWeightSum = 0;
        for (int i = start; i <= split; i++)
        {
            if (items[i]->marketCap >= m_minMarketCap) rowWeightSum += items[i]->marketCap;
        }

        double rowArea = remaining.width() * remaining.height() * (rowWeightSum / remainingWeight);
        double side = vertical ? remaining.width() : remaining.height();
        double otherSide = rowArea / side;

        // 分配这一行内每个股票的位置
        double offset = 0;
        for (int i = start; i <= split; i++)
        {
            // 太小的股票不显示（但通过items保留索引关系）
            if (items[i]->marketCap < m_minMarketCap)
            {
                items[i]->rect = QRectF();
                continue;
            }

            // 按市值比例分配长度
            double ratio = items[i]->marketCap / rowWeightSum;
            double size = side * ratio;

            // 设置股票在画布上的位置（QRectF）
            if (vertical)
            {
                items[i]->rect = QRectF(remaining.x() + offset, remaining.y(), size, otherSide);
            }
            else
            {
                items[i]->rect = QRectF(remaining.x(), remaining.y() + offset, otherSide, size);
            }
            offset += size;
        }

        // ---- 更新剩余区域 ----
        if (vertical)
        {
            remaining = QRectF(remaining.x(), remaining.y() + otherSide,
                               remaining.width(), remaining.height() - otherSide);
        }
        else
        {
            remaining = QRectF(remaining.x() + otherSide, remaining.y(),
                               remaining.width() - otherSide, remaining.height());
        }

        remainingWeight -= rowWeightSum;
        vertical = !vertical; // 切换切分方向，优化整体布局
        start = split + 1;
    }
}

/**
 * @brief 绘制事件（主渲染函数）
 * 按层级顺序绘制：背景 -> 板块背景 -> 个股色块 -> 板块标题 -> 边框 -> 文字 -> 提示框
 */
void TreeMapWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event) // 标记参数未使用，消除编译警告

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing); // 开启抗锯齿

    // 使用主题背景色
    p.fillRect(rect(), QColor(Tokens::Colors::BgBase));

    // 数据为空时显示提示
    if (m_visibleItems.isEmpty())
    {
        p.setPen(QColor(Tokens::Colors::TextPrimary));
        p.setFont(QFont("Microsoft YaHei", 14));
        p.drawText(rect(), Qt::AlignCenter, "暂无数据");
        return;
    }

    // 1. 绘制板块背景和边框（仅板块模式）
    if (m_layoutMode == ByIndustry)
    {
        drawIndustryBlocks(&p);
    }

    // 2. 绘制个股色块（红涨绿跌）
    drawTiles(&p);

    // 3. 绘制板块标题栏（覆盖在个股上层，确保可读）
    if (m_layoutMode == ByIndustry)
    {
        drawBlockLabels(&p);
    }

    // 4. 绘制边框和选中高亮
    drawBorders(&p);

    // 5. 绘制个股文字标签（名称和涨跌幅）
    drawLabels(&p);

    // 6. 绘制鼠标悬停提示框（最上层）
    drawTooltip(&p);
}

/**
 * @brief 绘制板块背景
 * 半透明背景色 + 主题色边框
 */
void TreeMapWidget::drawIndustryBlocks(QPainter* p)
{
    for (auto& block : m_industryBlocks)
    {
        if (!block.rect.isValid()) continue;

        // 板块背景：主题色极透明，用于视觉分组
        QColor bg = block.themeColor;
        bg.setAlpha(20); // 透明度约8%
        p->fillRect(block.rect, bg);

        // 板块边框：主题色较深
        QPen pen(block.themeColor.darker(150), 2);
        p->setPen(pen);
        p->drawRect(block.rect.adjusted(1, 1, -1, -1));

        // 鼠标悬停高亮（仅当悬停在板块空白处，非个股上）
        if (m_hoveredBlock == &block && !m_hoveredItem)
        {
            p->setPen(QPen(block.themeColor.lighter(120), 3));
            p->drawRect(block.rect.adjusted(2, 2, -2, -2));
        }
    }
}

/**
 * @brief 绘制板块标题栏
 * 显示：行业名称、(上涨家数/下跌家数)、板块平均涨跌幅
 */
void TreeMapWidget::drawBlockLabels(QPainter* p)
{
    for (auto& block : m_industryBlocks)
    {
        if (!block.rect.isValid()) continue;

        // 标题栏高度22像素，宽度同板块
        QRectF titleRect(block.rect.x(), block.rect.y(), block.rect.width(), 20);

        // 标题背景：主题色半透明
        QColor bg = block.themeColor;
        bg.setAlpha(180);
        p->fillRect(titleRect, bg);

        // 计算板块统计信息
        double avgChange = 0;
        int count = 0, upCount = 0, downCount = 0;
        for (auto item : block.items)
        {
            avgChange += item->changePercent;
            count++;
            if (item->changePercent > 0.01) upCount++;
            else if (item->changePercent < -0.01) downCount++;
        }
        if (count > 0) avgChange /= count;

        // 格式化标题：行业名 (涨/跌) 平均涨幅%
        QString title = QString("%1 (%2/%3) %4%")
                        .arg(block.name)
                        .arg(upCount)
                        .arg(downCount)
                        .arg(avgChange, 0, 'f', 2);

        // 绘制文字
        p->setPen(QColor(Tokens::Colors::TextPrimary));
        QFont font("Microsoft YaHei", 9, QFont::Bold);
        p->setFont(font);

        // 文字过长时截断
        QFontMetrics fm(font);
        QString elided = fm.elidedText(title, Qt::ElideRight, static_cast<int>(titleRect.width()) - 10);
        p->drawText(titleRect.adjusted(5, 0, -5, 0), Qt::AlignLeft | Qt::AlignVCenter, elided);

        // 标题栏下分隔线
        p->setPen(QPen(block.themeColor.darker(120), 1));
        p->drawLine(titleRect.bottomLeft(), titleRect.bottomRight());
    }
}

/**
 * @brief 绘制个股色块
 * 根据涨跌幅映射颜色，面积代表市值大小
 */
void TreeMapWidget::drawTiles(QPainter* p)
{
    for (auto item : m_visibleItems)
    {
        // 过滤无效或过小的矩形（小于2像素不绘制，避免性能问题）
        if (!item->rect.isValid() || item->rect.width() < 2 || item->rect.height() < 2)
            continue;

        // 获取涨跌对应的颜色（红涨绿跌）
        QColor color = interpolateColor(item->changePercent);

        // 选中项高亮（亮度提升30%）
        if (item == m_selectedItem)
        {
            color = color.lighter(130);
        }

        // 绘制填充色块
        p->fillRect(item->rect, color);
    }
}

/**
 * @brief 绘制边框
 * 细边框分隔个股，选中项加粗白框
 */
void TreeMapWidget::drawBorders(QPainter* p)
{
    QColor borderColor(Tokens::Colors::Border);

    // 默认细边框
    p->setPen(QPen(borderColor, 1));

    for (auto item : m_visibleItems)
    {
        if (!item->rect.isValid()) continue;
        p->drawRect(item->rect);

        // 选中项：使用主题强调色粗边框
        if (item == m_selectedItem)
        {
            p->setPen(QPen(QColor(Tokens::Colors::Primary), 3));
            p->drawRect(item->rect.adjusted(1, 1, -1, -1));
            p->setPen(QPen(borderColor, 1)); // 恢复默认
        }
    }
}

/**
 * @brief 绘制个股文字标签
 * 小方块只显示涨跌幅，大方块显示名称+涨跌幅
 */
void TreeMapWidget::drawLabels(QPainter* p)
{
    for (auto item : m_visibleItems)
    {
        if (!item->rect.isValid()) continue;

        // 过滤太小的矩形（无法容纳文字）
        if (item->rect.width() < 30 || item->rect.height() < 25) continue;

        QRectF r = item->rect;

        // 根据背景亮度决定文字颜色（黑或白，确保对比度）
        QColor bgColor = interpolateColor(item->changePercent);
        int brightness = (bgColor.red() * 299 + bgColor.green() * 587 + bgColor.blue() * 114) / 1000;
        p->setPen(brightness > 128 ? Qt::black : Qt::white);

        // 动态字体大小：根据矩形大小自适应，限制在8-11pt
        int fontSize = qBound(8, static_cast<int>(qMin(r.width() / 6, r.height() / 4)), 11);
        QFont font("Microsoft YaHei", fontSize, QFont::Bold);
        p->setFont(font);

        // 处理超长名称（省略号截断）
        QString name = item->name;
        QFontMetrics fm(font);
        if (fm.horizontalAdvance(name) > r.width() - 4)
        {
            name = fm.elidedText(name, Qt::ElideRight, static_cast<int>(r.width()) - 4);
        }

        // 涨跌幅格式化（保留1位小数）
        QString percent = QString::number(item->changePercent, 'f', 1) + "%";

        // 大方块：显示两行（名称在上，涨幅在下）
        if (r.height() > 40)
        {
            p->drawText(r.adjusted(2, 2, -2, -2), Qt::AlignTop | Qt::AlignHCenter, name);
            QFont bigFont = font;
            bigFont.setPointSize(fontSize + 1); // 涨幅字体稍大
            p->setFont(bigFont);
            p->drawText(r.adjusted(2, 2, -2, -2), Qt::AlignBottom | Qt::AlignHCenter, percent);
        }
        else
        {
            // 小方块：只显示涨幅
            p->drawText(r.adjusted(2, 2, -2, -2), Qt::AlignCenter, percent);
        }
    }
}

/**
 * @brief 绘制鼠标悬停提示框
 * 半透明黑底白字，显示详细信息
 */
void TreeMapWidget::drawTooltip(QPainter* p)
{
    if (!m_hoveredItem || !m_hoveredItem->rect.isValid()) return;

    // 准备提示文字内容
    QStringList lines;
    lines << QString("%1 %2").arg(m_hoveredItem->code).arg(m_hoveredItem->name);
    lines << QString("现价: %1").arg(m_hoveredItem->price, 0, 'f', 2);
    lines << QString("涨跌: %1%").arg(m_hoveredItem->changePercent, 0, 'f', 2);
    lines << QString("市值: %1亿").arg(m_hoveredItem->marketCap, 0, 'f', 2);

    // 期货额外显示持仓量
    if (m_hoveredItem->isFutures)
    {
        lines << QString("持仓: %1").arg(m_hoveredItem->openInterest, 0, 'f', 0);
    }

    // 计算提示框尺寸
    QFont font("Microsoft YaHei", 10);
    p->setFont(font);
    QFontMetrics fm(font);
    int maxWidth = 0;
    int lineHeight = fm.height();
    for (const QString& line : lines)
    {
        maxWidth = qMax(maxWidth, fm.horizontalAdvance(line));
    }

    // 框体大小：文字区 + 边距
    int padding = 8;
    QRectF tipRect(m_mousePos.x() + 15, m_mousePos.y() + 15, // 偏移鼠标位置15像素
                   maxWidth + padding * 2, lineHeight * lines.size() + padding * 2);

    // 边界检测：防止提示框超出窗口右/下边界
    if (tipRect.right() > rect().right()) tipRect.moveRight(m_mousePos.x() - 10);
    if (tipRect.bottom() > rect().bottom()) tipRect.moveBottom(m_mousePos.y() - 10);

    // 绘制半透明背景
    QColor bgColor(Tokens::Colors::BgElevated);
    bgColor.setAlpha(230);
    p->fillRect(tipRect, bgColor);
    p->setPen(QPen(QColor(Tokens::Colors::Border), 1));
    p->drawRect(tipRect);

    // 绘制文字
    p->setPen(QColor(Tokens::Colors::TextPrimary));
    int y = static_cast<int>(tipRect.top() + padding);
    for (const QString& line : lines)
    {
        p->drawText(static_cast<int>(tipRect.left() + padding), y + fm.ascent(), line);
        y += lineHeight;
    }
}

/**
 * @brief 颜色插值函数
 * 将涨跌幅映射为颜色（A股风格：红涨绿跌）
 * @param percent 涨跌幅百分比（如10.5表示涨10.5%）
 * @return 对应的QColor
 */
QColor TreeMapWidget::interpolateColor(double percent)
{
    QColor upColor(Tokens::Colors::ChartRed);
    QColor downColor(Tokens::Colors::ChartGreen);

    if (percent > 0)
    {
        // 上涨：使用主题上涨色，涨幅越大越饱和
        double ratio = qMin(percent / 10.0, 1.0); // 10%封顶
        int r = upColor.red();
        int g = upColor.green();
        int b = upColor.blue();

        // 从暗到亮渐变
        int baseR = static_cast<int>(r * 0.3);
        int baseG = static_cast<int>(g * 0.3);
        int baseB = static_cast<int>(b * 0.3);

        return QColor(
            baseR + static_cast<int>((r - baseR) * ratio),
            baseG + static_cast<int>((g - baseG) * ratio),
            baseB + static_cast<int>((b - baseB) * ratio)
        );
    }
    else if (percent < 0)
    {
        // 下跌：使用主题下跌色，跌幅越大越饱和
        double ratio = qMin(qAbs(percent) / 10.0, 1.0);
        int r = downColor.red();
        int g = downColor.green();
        int b = downColor.blue();

        // 从暗到亮渐变
        int baseR = static_cast<int>(r * 0.3);
        int baseG = static_cast<int>(g * 0.3);
        int baseB = static_cast<int>(b * 0.3);

        return QColor(
            baseR + static_cast<int>((r - baseR) * ratio),
            baseG + static_cast<int>((g - baseG) * ratio),
            baseB + static_cast<int>((b - baseB) * ratio)
        );
    }
    else
    {
        // 平盘：使用主题边框色作为中性色
        return QColor(Tokens::Colors::Border);
    }
}

/**
 * @brief 更新统计信息并发送信号
 * 计算总涨跌家数和成交额，供状态栏显示
 */
void TreeMapWidget::updateStats()
{
    int up = 0, flat = 0, down = 0;
    double totalTurnover = 0;

    // 基于原始全部数据统计（不是筛选后的）
    for (const auto& item : m_allItems)
    {
        if (item.changePercent > 0.01) up++;
        else if (item.changePercent < -0.01) down++;
        else flat++;
        totalTurnover += item.turnover;
    }

    // 成交额转换为亿元，发送信号给DashboardPage更新状态栏
    emit statsChanged(up, flat, down, totalTurnover / 10000.0);
}

/**
 * @brief 获取指定坐标的股票
 * 从后向前遍历（后绘制的在上面，优先选中）
 * @param pos 鼠标坐标
 * @return 该位置的股票指针，无则返回nullptr
 */
StockQuoteItem* TreeMapWidget::itemAt(const QPoint& pos)
{
    for (int i = m_visibleItems.size() - 1; i >= 0; --i)
    {
        if (m_visibleItems[i]->rect.contains(pos))
        {
            return m_visibleItems[i];
        }
    }
    return nullptr;
}

/**
 * @brief 获取指定坐标的板块
 * @param pos 鼠标坐标
 * @return 该位置的板块指针，无则返回nullptr
 */
IndustryBlock* TreeMapWidget::blockAt(const QPoint& pos)
{
    for (auto& block : m_industryBlocks)
    {
        if (block.rect.contains(pos))
        {
            return &block;
        }
    }
    return nullptr;
}

/**
 * @brief 鼠标按下事件
 * 处理点击选中和板块点击
 */
void TreeMapWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        // 先检测是否点中个股
        StockQuoteItem* item = itemAt(event->pos());

        if (item)
        {
            m_selectedItem = item;
            m_selectedIndex = m_visibleItems.indexOf(item);
            emit itemClicked(*item); // 发送信号给外部（如DashboardPage）
            update();
        }
        else
        {
            // 未点中个股，检测是否点中板块（空白处）
            IndustryBlock* block = blockAt(event->pos());
            if (block)
            {
                emit blockClicked(block->name); // 发送板块点击信号
            }
            else
            {
                // 点击空白处，清空选择
                m_selectedItem = nullptr;
                m_selectedIndex = -1;
                update();
            }
        }
    }
}

/**
 * @brief 鼠标双击事件
 * 打开详情（K线图或详细信息窗口）
 */
void TreeMapWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    StockQuoteItem* item = itemAt(event->pos());
    if (item)
    {
        emit itemDoubleClicked(*item);
    }
}

/**
 * @brief 鼠标移动事件
 * 处理悬停效果和提示框位置
 */
void TreeMapWidget::mouseMoveEvent(QMouseEvent* event)
{
    m_mousePos = event->pos();
    StockQuoteItem* item = itemAt(m_mousePos);

    // 个股悬停状态变化
    if (item != m_hoveredItem)
    {
        m_hoveredItem = item;

        if (item)
        {
            setCursor(Qt::PointingHandCursor); // 手型光标表示可点击
            emit itemHovered(*item); // 发送悬停信号（外部可更新状态栏）
        }
        else
        {
            setCursor(Qt::ArrowCursor); // 恢复箭头
        }
        update(); // 需要重绘以显示/隐藏提示框
    }
    else if (m_hoveredItem)
    {
        // 即使item没变，鼠标移动了也要重绘提示框（跟随鼠标）
        update();
    }

    // 板块悬停检测（仅当未悬停个股时）
    if (m_layoutMode == ByIndustry && !m_hoveredItem)
    {
        IndustryBlock* block = blockAt(m_mousePos);
        if (block != m_hoveredBlock)
        {
            m_hoveredBlock = block;
            if (block)
            {
                // 显示板块统计提示
                setToolTip(QString("板块: %1\n股票数: %2\n总市值: %3亿")
                           .arg(block->name)
                           .arg(block->items.size())
                           .arg(block->totalMarketCap, 0, 'f', 2));
            }
            else
            {
                setToolTip("");
            }
            update();
        }
    }
}

/**
 * @brief 鼠标离开事件
 * 清除悬停状态
 */
void TreeMapWidget::leaveEvent(QEvent* event)
{
    Q_UNUSED(event);
    m_hoveredItem = nullptr;
    m_hoveredBlock = nullptr;
    setCursor(Qt::ArrowCursor);
    update();
}

/**
 * @brief 键盘事件
 * 方向键导航，ESC清空选择
 */
void TreeMapWidget::keyPressEvent(QKeyEvent* event)
{
    switch (event->key())
    {
    case Qt::Key_Right:
    case Qt::Key_Down:
        navigateNext(); // 选择下一个
        break;
    case Qt::Key_Left:
    case Qt::Key_Up:
        navigatePrevious(); // 选择上一个
        break;
    case Qt::Key_Escape:
        clearSelection(); // ESC清空选择
        break;
    default:
        QWidget::keyPressEvent(event); // 其他键默认处理
    }
}

/**
 * @brief 滚轮事件
 * Ctrl+滚轮调整最小显示市值（过滤小股票）
 */
void TreeMapWidget::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        // Ctrl按下时：调整市值过滤阈值
        double delta = event->angleDelta().y() > 0 ? -0.5 : 0.5;
        m_minMarketCap = qMax(0.0, m_minMarketCap + delta);
        calculateLayout(); // 重新计算布局（过滤小市值）
        update();
    }
    else
    {
        event->ignore(); // 普通滚轮事件忽略，可传递给父窗口（如滚动页面）
    }
}

/**
 * @brief 窗口尺寸变化事件
 * 重新计算布局以适应新尺寸
 */
void TreeMapWidget::resizeEvent(QResizeEvent* event)
{
    Q_UNUSED(event);
    calculateLayout(); // 尺寸变化必须重算布局
}

/**
 * @brief 内部导航实现（供navigateNext/navigatePrevious调用）
 * @param direction 1=下一个，-1=上一个
 */
void TreeMapWidget::selectNextItem(int direction)
{
    if (m_visibleItems.isEmpty()) return;

    m_selectedIndex += direction;
    // 循环导航
    if (m_selectedIndex < 0) m_selectedIndex = m_visibleItems.size() - 1;
    if (m_selectedIndex >= m_visibleItems.size()) m_selectedIndex = 0;

    m_selectedItem = m_visibleItems[m_selectedIndex];
    emit itemClicked(*m_selectedItem);
    update();
}

/**
 * @brief 加载历史数据帧（复盘功能）
 * @param frameIndex 历史数据索引
 */
void TreeMapWidget::loadHistoryFrame(int frameIndex)
{
    if (frameIndex < 0 || frameIndex >= m_historyData.size()) return;

    const auto& frame = m_historyData[frameIndex];

    // 更新当前数据为历史帧
    for (int i = 0; i < m_allItems.size(); ++i)
    {
        QString code = m_allItems[i].code;
        if (frame.contains(code))
        {
            m_allItems[i] = frame[code];
        }
    }

    // 复盘模式下保持原有分组和布局结构，只更新数值
    if (m_layoutMode == ByIndustry)
    {
        // 重新计算各板块总市值（因为个股价格变了）
        for (auto& block : m_industryBlocks)
        {
            block.totalMarketCap = 0;
            for (auto item : block.items)
            {
                block.totalMarketCap += item->marketCap; // 注意：这里应该用当前价计算的最新市值
            }
        }
    }

    calculateLayout();
    update();
    updateStats();
}
