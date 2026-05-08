/**
 * @file PluginSystem.cpp
 * @brief 插件系统实现
 */

#include "PluginSystem.h"
#include "utils/Logger.h"
#include <QUuid>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

PluginSystem* PluginSystem::instance()
{
    static PluginSystem* inst = new PluginSystem();
    return inst;
}

PluginSystem::PluginSystem(QObject* parent)
    : QObject(parent)
    , m_pythonPath(QStringLiteral("python"))
{
}

bool PluginSystem::initialize()
{
    if (m_initialized) return true;

    LOG_INFO("Initializing Plugin System");

    // 检测Python环境
    QProcess process;
    process.start(m_pythonPath, QStringList() << QStringLiteral("--version"));
    if (process.waitForFinished(5000)) {
        QString version = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        LOG_INFO(QString("Python detected: %1").arg(version));
    } else {
        LOG_WARNING("Python not found, plugin system will run in limited mode");
    }

    // 扫描插件目录
    QString pluginDir = QDir::currentPath() + QStringLiteral("/plugins");
    if (!QDir(pluginDir).exists()) {
        QDir().mkpath(pluginDir);
        LOG_INFO(QString("Created plugin directory: %1").arg(pluginDir));
    }

    // 扫描已有插件
    QDir dir(pluginDir);
    QStringList filters;
    filters << QStringLiteral("*.py");
    QStringList scripts = dir.entryList(filters, QDir::Files);

    for (const QString& script : scripts) {
        QString path = dir.absoluteFilePath(script);
        installPlugin(path);
    }

    m_initialized = true;
    LOG_INFO("Plugin System initialized");
    return true;
}

bool PluginSystem::installPlugin(const QString& scriptPath)
{
    if (!QFile::exists(scriptPath)) {
        LOG_ERROR(QString("Script file not found: %1").arg(scriptPath));
        return false;
    }

    // 解析插件元数�?    PluginInfo info;
    if (!parsePluginMetadata(scriptPath, info)) {
        LOG_ERROR(QString("Failed to parse plugin metadata: %1").arg(scriptPath));
        return false;
    }

    info.id = generatePluginId();
    info.scriptPath = scriptPath;
    info.installTime = QDateTime::currentDateTime();
    info.updateTime = QDateTime::currentDateTime();

    // 验证脚本
    QString errorMsg;
    if (!validateScript(scriptPath, errorMsg)) {
        LOG_ERROR(QString("Script validation failed: %1 - %2").arg(scriptPath, errorMsg));
        return false;
    }

    m_plugins[info.id] = info;

    LOG_INFO(QString("Plugin installed: %1 (%2)").arg(info.name, info.id));
    emit pluginInstalled(info);

    return true;
}

bool PluginSystem::uninstallPlugin(const QString& pluginId)
{
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    PluginInfo info = m_plugins[pluginId];
    m_plugins.remove(pluginId);

    LOG_INFO(QString("Plugin uninstalled: %1").arg(info.name));
    emit pluginUninstalled(pluginId);

    return true;
}

void PluginSystem::setPluginEnabled(const QString& pluginId, bool enabled)
{
    if (m_plugins.contains(pluginId)) {
        m_plugins[pluginId].enabled = enabled;
        emit pluginEnabledChanged(pluginId, enabled);
    }
}

QVector<PluginInfo> PluginSystem::getPlugins(PluginType type) const
{
    QVector<PluginInfo> result;

    for (const auto& plugin : m_plugins) {
        if (type == plugin.type) {
            result.append(plugin);
        }
    }

    return result;
}

PluginInfo PluginSystem::getPluginInfo(const QString& pluginId) const
{
    return m_plugins.value(pluginId);
}

IndicatorResult PluginSystem::calculateIndicator(const QString& pluginId,
                                                  const PluginContext& context)
{
    IndicatorResult result;

    if (!m_plugins.contains(pluginId)) {
        result.name = QStringLiteral("Error");
        return result;
    }

    const PluginInfo& plugin = m_plugins[pluginId];

    if (!plugin.enabled) {
        result.name = plugin.name + QStringLiteral(" (disabled)");
        return result;
    }

    // 构建Python脚本
    QString script = buildScript(plugin.scriptPath, context);

    // 执行Python脚本
    QVariantMap params;
    params[QStringLiteral("symbol")] = context.symbol;
    params[QStringLiteral("action")] = QStringLiteral("calculate");

    QVariantMap output;
    if (executePython(script, params, output)) {
        result.name = output.value(QStringLiteral("name"), plugin.name).toString();

        // 解析指标�?        QVariantList values = output.value(QStringLiteral("values")).toList();
        for (const QVariant& v : values) {
            result.values.append(v.toDouble());
        }

        result.color = output.value(QStringLiteral("color"), QStringLiteral("#58a6ff")).toString();
        result.lineWidth = output.value(QStringLiteral("lineWidth"), 1).toInt();
        result.lineStyle = output.value(QStringLiteral("lineStyle"), QStringLiteral("solid")).toString();
        result.visible = output.value(QStringLiteral("visible"), true).toBool();
    } else {
        result.name = plugin.name + QStringLiteral(" (error)");
        emit pluginError(pluginId, QStringLiteral("Execution failed"));
    }

    return result;
}

QMap<QString, IndicatorResult> PluginSystem::calculateIndicators(
    const QVector<QString>& pluginIds,
    const PluginContext& context)
{
    QMap<QString, IndicatorResult> results;

    for (const QString& pluginId : pluginIds) {
        results[pluginId] = calculateIndicator(pluginId, context);
    }

    return results;
}

bool PluginSystem::validateScript(const QString& scriptPath, QString& errorMsg)
{
    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly)) {
        errorMsg = QStringLiteral("Cannot open file");
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 检查必须的函数
    if (!content.contains(QStringLiteral("def calculate"))) {
        errorMsg = QStringLiteral("Missing 'calculate' function");
        return false;
    }

    // 检查元数据
    if (!content.contains(QStringLiteral("@plugin"))) {
        errorMsg = QStringLiteral("Missing @plugin decorator");
        return false;
    }

    // 使用Python验证语法
    QProcess process;
    process.start(m_pythonPath, QStringList()
        << QStringLiteral("-c")
        << QString(QStringLiteral("import py_compile; py_compile.compile('%1', doraise=True)")).arg(scriptPath));

    if (!process.waitForFinished(5000)) {
        errorMsg = QStringLiteral("Python validation timeout");
        return false;
    }

    if (process.exitCode() != 0) {
        errorMsg = QString::fromUtf8(process.readAllStandardError()).trimmed();
        return false;
    }

    return true;
}

QString PluginSystem::getScriptTemplate(PluginType type) const
{
    QString template_;

    if (type == PluginType::Indicator) {
        template_ = QStringLiteral(
            "#!/usr/bin/env python3\n"
            "# -*- coding: utf-8 -*-\n"
            "\"\"\"\n"
            "@plugin\n"
            "name: 我的自定义指标\n"
            "version: 1.0.0\n"
            "author: WealthPilot User\n"
            "description: 自定义指标计算\n"
            "type: indicator\n"
            "\"\"\"\n"
            "\n"
            "def calculate(prices, highs, lows, opens, volumes, **params):\n"
            "    \"\"\"\n"
            "    计算自定义指标\n"
            "    \n"
            "    参数:\n"
            "        prices: 收盘价数组\n"
            "        highs: 最高价数组\n"
            "        lows: 最低价数组\n"
            "        opens: 开盘价数组\n"
            "        volumes: 成交量数组\n"
            "        **params: 自定义参数\n"
            "    \n"
            "    返回:\n"
            "        dict: {\n"
            "            'name': '指标名称',\n"
            "            'values': [指标值数组],\n"
            "            'color': '#58a6ff',\n"
            "            'lineWidth': 1,\n"
            "            'lineStyle': 'solid',\n"
            "            'visible': True\n"
            "        }\n"
            "    \"\"\"\n"
            "    # 示例：简单移动平均线\n"
            "    period = params.get('period', 20)\n"
            "    \n"
            "    values = []\n"
            "    for i in range(len(prices)):\n"
            "        if i < period - 1:\n"
            "            values.append(None)\n"
            "        else:\n"
            "            ma = sum(prices[i-period+1:i+1]) / period\n"
            "            values.append(ma)\n"
            "    \n"
            "    return {\n"
            "        'name': f'MA{period}',\n"
            "        'values': values,\n"
            "        'color': '#58a6ff',\n"
            "        'lineWidth': 1,\n"
            "        'lineStyle': 'solid',\n"
            "        'visible': True\n"
            "    }\n"
        );
    } else if (type == PluginType::Strategy) {
        template_ = QStringLiteral(
            "#!/usr/bin/env python3\n"
            "# -*- coding: utf-8 -*-\n"
            "\"\"\"\n"
            "@plugin\n"
            "name: 我的自定义策略\n"
            "version: 1.0.0\n"
            "author: WealthPilot User\n"
            "description: 自定义交易策略\n"
            "type: strategy\n"
            "\"\"\"\n"
            "\n"
            "def evaluate(prices, highs, lows, opens, volumes, **params):\n"
            "    \"\"\"\n"
            "    评估策略信号\n"
            "    \n"
            "    返回:\n"
            "        dict: {\n"
            "            'signal': 'buy' | 'sell' | 'hold',\n"
            "            'strength': 0-100,\n"
            "            'reason': '信号原因'\n"
            "        }\n"
            "    \"\"\"\n"
            "    # 示例：简单均线策略\n"
            "    if len(prices) < 20:\n"
            "        return {'signal': 'hold', 'strength': 0, 'reason': '数据不足'}\n"
            "    \n"
            "    ma5 = sum(prices[-5:]) / 5\n"
            "    ma20 = sum(prices[-20:]) / 20\n"
            "    \n"
            "    if ma5 > ma20:\n"
            "        return {'signal': 'buy', 'strength': 70, 'reason': 'MA5上穿MA20'}\n"
            "    elif ma5 < ma20:\n"
            "        return {'signal': 'sell', 'strength': 70, 'reason': 'MA5下穿MA20'}\n"
            "    else:\n"
            "        return {'signal': 'hold', 'strength': 30, 'reason': '无明确信�?}\n"
        );
    } else if (type == PluginType::Alert) {
        template_ = QStringLiteral(
            "#!/usr/bin/env python3\n"
            "# -*- coding: utf-8 -*-\n"
            "\"\"\"\n"
            "@plugin\n"
            "name: 我的自定义预警\n"
            "version: 1.0.0\n"
            "author: WealthPilot User\n"
            "description: 自定义预警条件\n"
            "type: alert\n"
            "\"\"\"\n"
            "\n"
            "def check(prices, highs, lows, opens, volumes, **params):\n"
            "    \"\"\"\n"
            "    检查预警条件\n"
            "    \n"
            "    返回:\n"
            "        dict: {\n"
            "            'triggered': True/False,\n"
            "            'message': '预警消息',\n"
            "            'level': 'low' | 'medium' | 'high' | 'critical'\n"
            "        }\n"
            "    \"\"\"\n"
            "    threshold = params.get('threshold', 5.0)\n"
            "    \n"
            "    if len(prices) < 2:\n"
            "        return {'triggered': False, 'message': '', 'level': 'low'}\n"
            "    \n"
            "    change = (prices[-1] - prices[-2]) / prices[-2] * 100\n"
            "    \n"
            "    if abs(change) >= threshold:\n"
            "        direction = '上涨' if change > 0 else '下跌'\n"
            "        return {\n"
            "            'triggered': True,\n"
            "            'message': f'股价{direction}{abs(change):.2f}%',\n"
            "            'level': 'high' if abs(change) >= threshold * 2 else 'medium'\n"
            "        }\n"
            "    \n"
            "    return {'triggered': False, 'message': '', 'level': 'low'}\n"
        );
    }

    return template_;
}

void PluginSystem::setPythonPath(const QString& path)
{
    m_pythonPath = path;
    LOG_INFO(QString("Python path set to: %1").arg(path));
}

bool PluginSystem::executePython(const QString& script, const QVariantMap& params, QVariantMap& result)
{
    QProcess process;

    // 构建JSON输入
    QJsonObject inputObj;
    for (auto it = params.begin(); it != params.end(); ++it) {
        inputObj[it.key()] = QJsonValue::fromVariant(it.value());
    }

    QString inputJson = QString::fromUtf8(QJsonDocument(inputObj).toJson());

    // 执行Python脚本
    process.start(m_pythonPath, QStringList() << QStringLiteral("-c") << script);

    if (!process.waitForStarted(5000)) {
        LOG_ERROR("Failed to start Python process");
        return false;
    }

    // 发送输入数�?    process.write(inputJson.toUtf8());
    process.closeWriteChannel();

    if (!process.waitForFinished(30000)) {
        LOG_ERROR("Python process timeout");
        process.kill();
        return false;
    }

    if (process.exitCode() != 0) {
        QString error = QString::fromUtf8(process.readAllStandardError());
        LOG_ERROR(QString("Python error: %1").arg(error));
        return false;
    }

    // 解析输出
    QByteArray output = process.readAllStandardOutput();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(output, &error);

    if (error.error != QJsonParseError::NoError) {
        LOG_ERROR(QString("JSON parse error: %1").arg(error.errorString()));
        return false;
    }

    result = doc.object().toVariantMap();
    return true;
}

QString PluginSystem::buildScript(const QString& scriptPath, const PluginContext& context)
{
    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QString script = QString::fromUtf8(file.readAll());
    file.close();

    // 包装脚本，添加数据输�?    QString wrapper = QStringLiteral(
        "import json\n"
        "import sys\n"
        "\n"
        "def main():\n"
        "    # 读取输入数据\n"
        "    input_data = json.loads(sys.stdin.read())\n"
        "    params = input_data.get('params', {})\n"
        "\n"
        "    # 提供价格数据\n"
        "    prices = %1\n"
        "    highs = %2\n"
        "    lows = %3\n"
        "    opens = %4\n"
        "    volumes = %5\n"
        "\n"
        "    # 执行用户脚本\n"
        "    result = calculate(prices, highs, lows, opens, volumes, **params)\n"
        "\n"
        "    # 输出结果\n"
        "    print(json.dumps(result))\n"
        "\n"
        "%6\n"
        "\n"
        "main()\n"
    );

    // 转换数据为Python列表
    auto toPyList = [](const QVector<double>& data) -> QString {
        QStringList items;
        for (double v : data) {
            items.append(QString::number(v, 'f', 4));
        }
        return QStringLiteral("[") + items.join(QStringLiteral(", ")) + QStringLiteral("]");
    };

    auto toPyIntList = [](const QVector<qint64>& data) -> QString {
        QStringList items;
        for (qint64 v : data) {
            items.append(QString::number(v));
        }
        return QStringLiteral("[") + items.join(QStringLiteral(", ")) + QStringLiteral("]");
    };

    return wrapper
        .arg(toPyList(context.prices))
        .arg(toPyList(context.highs))
        .arg(toPyList(context.lows))
        .arg(toPyList(context.opens))
        .arg(toPyIntList(context.volumes))
        .arg(script);
}

QString PluginSystem::generatePluginId() const
{
    return QStringLiteral("plugin_") + QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

bool PluginSystem::parsePluginMetadata(const QString& scriptPath, PluginInfo& info)
{
    QFile file(scriptPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QString content = QString::fromUtf8(file.readAll());
    file.close();

    // 解析文档字符串中的元数据
    QRegularExpression nameRe(QStringLiteral("@plugin\\s*\\n.*?name:\\s*(.+)"));
    QRegularExpression versionRe(QStringLiteral("version:\\s*(.+)"));
    QRegularExpression authorRe(QStringLiteral("author:\\s*(.+)"));
    QRegularExpression descRe(QStringLiteral("description:\\s*(.+)"));
    QRegularExpression typeRe(QStringLiteral("type:\\s*(.+)"));

    auto match = nameRe.match(content);
    if (match.hasMatch()) {
        info.name = match.captured(1).trimmed();
    } else {
        info.name = QFileInfo(scriptPath).baseName();
    }

    match = versionRe.match(content);
    info.version = match.hasMatch() ? match.captured(1).trimmed() : QStringLiteral("1.0.0");

    match = authorRe.match(content);
    info.author = match.hasMatch() ? match.captured(1).trimmed() : QStringLiteral("Unknown");

    match = descRe.match(content);
    info.description = match.hasMatch() ? match.captured(1).trimmed() : QString();

    match = typeRe.match(content);
    if (match.hasMatch()) {
        QString typeStr = match.captured(1).trimmed().toLower();
        if (typeStr == QStringLiteral("indicator")) {
            info.type = PluginType::Indicator;
        } else if (typeStr == QStringLiteral("strategy")) {
            info.type = PluginType::Strategy;
        } else if (typeStr == QStringLiteral("alert")) {
            info.type = PluginType::Alert;
        } else {
            info.type = PluginType::Utility;
        }
    } else {
        info.type = PluginType::Indicator;
    }

    return true;
}
