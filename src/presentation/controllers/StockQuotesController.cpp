/**
 * @file StockQuotesController.cpp
 * @brief 股票行情控制器实现
 * 
 * @author WealthPilot Team
 * @version 1.0.0
 */

#include "StockQuotesController.h"
#include "data/market/StockDataSource.h"
#include "data/models/StockQuoteItem.h"
#include "data/models/StockQuoteModel.h"
#include "shared/utils/Logger.h"

#include <QSortFilterProxyModel>
#include <QFile>
#include <QTextStream>
#include <QStringConverter>
#include <QClipboard>
#include <QApplication>
#include <QRegularExpression>

namespace WealthPilot
{
    StockQuotesController::StockQuotesController(QObject* parent)
        : ControllerBase(parent)
          , m_proxyModel(new QSortFilterProxyModel(this))
    {
        LOG_DEBUG("StockQuotesController created");
    }

    StockQuotesController::~StockQuotesController()
    {
        cleanup();
        LOG_DEBUG("StockQuotesController destroyed");
    }

    void StockQuotesController::initialize()
    {
        ControllerBase::initialize();

        // 获取数据源
        m_dataSource = getService<StockDataSource>();

        if (!m_dataSource)
        {
            setError("StockDataSource not available");
            return;
        }

        // 创建模型
        m_model = new StockQuoteModel(this);
        m_proxyModel->setSourceModel(m_model);
        m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        m_proxyModel->setSortRole(Qt::UserRole);

        // 连接数据源信号
        connect(m_dataSource, &StockDataSource::quotesReceived,
                this, &StockQuotesController::onDataReceived);
        connect(m_dataSource, &StockDataSource::errorOccurred,
                this, &StockQuotesController::onDataError);

        LOG_INFO("StockQuotesController initialized");
    }

    void StockQuotesController::cleanup()
    {
        ControllerBase::cleanup();

        if (m_dataSource)
        {
            disconnect(m_dataSource, nullptr, this, nullptr);
        }

        LOG_INFO("StockQuotesController cleaned up");
    }

    // ========== 数据操作 ==========

    void StockQuotesController::refreshData()
    {
        beginOperation("刷新数据");
        emit dataLoading(true);

        if (m_dataSource)
        {
            // 请求股票列表行情
            QStringList symbols;
            for (const auto& quote : m_allQuotes)
            {
                symbols << quote.symbol;
            }
            if (!symbols.isEmpty())
            {
                m_dataSource->requestQuotes(symbols);
            }
            else
            {
                // 默认请求一些常用股票
                m_dataSource->requestQuotes({"sh600000", "sh600519", "sz000001", "sz000002"});
            }
        }
        else
        {
            setError("数据源不可用");
            endOperation("刷新数据", false);
            emit dataLoading(false);
        }
    }

    void StockQuotesController::searchData(const QString& keyword)
    {
        m_searchKeyword = keyword.trimmed();
        applyFilter();

        int visibleCount = m_proxyModel->rowCount();
        emit searchCompleted(visibleCount, m_searchKeyword);

        LOG_DEBUG(QString("Search completed: keyword=%1, results=%2")
                  .arg(m_searchKeyword).arg(visibleCount));
    }

    void StockQuotesController::filterByMarket(const QString& market)
    {
        m_marketFilter = market;
        applyFilter();

        int visibleCount = m_proxyModel->rowCount();
        emit dataFiltered(visibleCount, m_allQuotes.size());

        LOG_DEBUG(QString("Filter by market: %1, visible=%2")
                  .arg(market).arg(visibleCount));
    }

    void StockQuotesController::filterByChange(const QString& filter)
    {
        m_changeFilter = filter;
        applyFilter();

        int visibleCount = m_proxyModel->rowCount();
        emit dataFiltered(visibleCount, m_allQuotes.size());

        LOG_DEBUG(QString("Filter by change: %1, visible=%2")
                  .arg(filter).arg(visibleCount));
    }

    void StockQuotesController::sortByColumn(int column, Qt::SortOrder order)
    {
        m_proxyModel->sort(column, order);
        LOG_DEBUG(QString("Sort by column %1, order=%2").arg(column).arg(order));
    }

    void StockQuotesController::clearData()
    {
        m_allQuotes.clear();
        if (m_model)
        {
            m_model->clear();
        }

        m_searchKeyword.clear();
        m_marketFilter = "all";
        m_changeFilter = "all";

        LOG_DEBUG("Data cleared");
    }

    // ========== 导出功能 ==========

    void StockQuotesController::exportToCSV(const QString& filePath)
    {
        beginOperation("导出CSV");

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QString error = QString("无法打开文件: %1").arg(filePath);
            setError(error);
            emit exportFailed(error);
            endOperation("导出CSV", false);
            return;
        }

        QTextStream out(&file);
        out.setEncoding(QStringConverter::Utf8);

        // 写入表头
        out << "代码,名称,最新价,涨跌额,涨跌幅,成交量,成交额,最高,最低\n";

        // 写入数据
        for (int i = 0; i < m_proxyModel->rowCount(); ++i)
        {
            QStringList row;
            for (int j = 0; j < m_proxyModel->columnCount(); ++j)
            {
                QModelIndex index = m_proxyModel->index(i, j);
                row << m_proxyModel->data(index).toString();
            }
            out << row.join(",") << "\n";
        }

        file.close();

        emit exportCompleted(filePath);
        endOperation("导出CSV", true);

        LOG_INFO(QString("Exported to CSV: %1").arg(filePath));
    }

    void StockQuotesController::exportToExcel(const QString& filePath)
    {
        // 简化实现：导出为 CSV 格式，用户可用 Excel 打开
        exportToCSV(filePath);
    }

    void StockQuotesController::copyToClipboard(const QModelIndexList& indices)
    {
        if (indices.isEmpty())
        {
            return;
        }

        QStringList rows;
        for (const QModelIndex& index : indices)
        {
            if (index.column() == 0)
            {
                // 只处理每行第一列
                QStringList rowData;
                for (int col = 0; col < m_proxyModel->columnCount(); ++col)
                {
                    QModelIndex cellIndex = m_proxyModel->index(index.row(), col);
                    rowData << m_proxyModel->data(cellIndex).toString();
                }
                rows << rowData.join("\t");
            }
        }

        QApplication::clipboard()->setText(rows.join("\n"));
        LOG_DEBUG(QString("Copied %1 rows to clipboard").arg(rows.size()));
    }

    // ========== 统计信息 ==========

    int StockQuotesController::totalCount() const
    {
        return m_allQuotes.size();
    }

    int StockQuotesController::filteredCount() const
    {
        return m_proxyModel->rowCount();
    }

    QVariantMap StockQuotesController::getChangeStatistics() const
    {
        int upCount = 0;
        int downCount = 0;
        int flatCount = 0;

        for (const auto& quote : m_allQuotes)
        {
            if (quote.changePercent > 0.01)
            {
                upCount++;
            }
            else if (quote.changePercent < -0.01)
            {
                downCount++;
            }
            else
            {
                flatCount++;
            }
        }

        return {
            {"up", upCount},
            {"down", downCount},
            {"flat", flatCount},
            {"total", m_allQuotes.size()}
        };
    }

    // ========== 私有方法 ==========

    void StockQuotesController::applyFilter()
    {
        // 构建正则表达式
        QString pattern;

        // 市场筛选
        if (m_marketFilter != "all")
        {
            if (m_marketFilter == "sh")
            {
                pattern = "^6[0-9]{5}$";
            }
            else if (m_marketFilter == "sz")
            {
                pattern = "^(000|002|300)[0-9]{3}$";
            }
            else if (m_marketFilter == "cyb")
            {
                pattern = "^300[0-9]{3}$";
            }
            else if (m_marketFilter == "kcb")
            {
                pattern = "^688[0-9]{3}$";
            }
        }

        // 设置筛选
        if (!pattern.isEmpty())
        {
            m_proxyModel->setFilterRegularExpression(QRegularExpression(pattern));
        }
        else
        {
            m_proxyModel->setFilterRegularExpression(QRegularExpression());
        }

        // 搜索筛选
        if (!m_searchKeyword.isEmpty())
        {
            m_proxyModel->setFilterFixedString(m_searchKeyword);
        }

        // 涨跌筛选（需要自定义筛选器）
        if (m_changeFilter != "all")
        {
            // 这里需要实现自定义筛选逻辑
            // 暂时简化处理
        }
    }

    // ========== 信号处理 ==========

    void StockQuotesController::onDataReceived(const QVector<StockQuote>& quotes)
    {
        m_allQuotes = quotes;

        if (m_model)
        {
            m_model->setData(quotes);
        }

        emit dataRefreshed(quotes.size());
        emit dataLoading(false);
        endOperation("刷新数据", true);

        LOG_INFO(QString("Data received: %1 quotes").arg(quotes.size()));
    }

    void StockQuotesController::onDataError(const QString& error)
    {
        setError(error);
        emit dataLoading(false);
        endOperation("刷新数据", false);

        LOG_ERROR(QString("Data error: %1").arg(error));
    }
} // namespace WealthPilot