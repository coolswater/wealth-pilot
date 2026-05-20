/**
 * @file PluginSystem.cpp
 * @brief 鎻掍欢绯荤粺瀹炵幇
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

    // 妫€娴婸ython鐜
    QProcess process;
    process.start(m_pythonPath, QStringList() << QStringLiteral("--version"));
    if (process.waitForFinished(5000)) {
        QString version = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        LOG_INFO(QString("Python detected: %1").arg(version));
    } else {
        LOG_WARNING("Python not found, plugin system will run in limited mode");
    }

    // 鎵弿鎻掍欢鐩綍
    QString pluginDir = QDir::currentPath() + QStringLiteral("/plugins");
    if (!QDir(pluginDir).exists()) {
        QDir().mkpath(pluginDir);
        LOG_INFO(QString("Created plugin directory: %1").arg(pluginDir));
    }

    // 鎵弿宸叉湁鎻掍欢
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

    // 瑙ｆ瀽鎻掍欢鍏冩暟鎹?    PluginInfo info;
    if (!parsePluginMetadata(scriptPath, info)) {
        LOG_ERROR(QString("Failed to parse plugin metadata: %1").arg(scriptPath));
        return false;
    }

    info.id = generatePluginId();
    info.scriptPath = scriptPath;
    info.installTime = QDateTime::currentDateTime();
    info.updateTime = QDateTime::currentDateTime();

    // 楠岃瘉鑴氭湰
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

    // 鏋勫缓Python鑴氭湰
    QString script = buildScript(plugin.scriptPath, context);

    // 鎵цPython鑴氭湰
    QVariantMap params;
    params[QStringLiteral("symbol")] = context.symbol;
    params[QStringLiteral("action")] = QStringLiteral("calculate");

    QVariantMap output;
    if (executePython(script, params, output)) {
        result.name = output.value(QStringLiteral("name"), plugin.name).toString();

        // 瑙ｆ瀽鎸囨爣鍊?        QVariantList values = output.value(QStringLiteral("values")).toList();
        for (const QVariant& v : values) {
            result.values.append(v.toDouble());
        }

        result.color = output.value(QStringLiteral("color"), QStringLiteral("#3B82F6")).toString();
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

    // 妫€鏌ュ繀椤荤殑鍑芥暟
    if (!content.contains(QStringLiteral("def calculate"))) {
        errorMsg = QStringLiteral("Missing 'calculate' function");
        return false;
    }

    // 妫€鏌ュ厓鏁版嵁
    if (!content.contains(QStringLiteral("@plugin"))) {
        errorMsg = QStringLiteral("Missing @plugin decorator");
        return false;
    }

    // 浣跨敤Python楠岃瘉璇硶
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
            "name: 鎴戠殑鑷畾涔夋寚鏍嘰n"
            "version: 1.0.0\n"
            "author: WealthPilot User\n"
            "description: 鑷畾涔夋寚鏍囪绠梊n"
            "type: indicator\n"
            "\"\"\"\n"
            "\n"
            "def calculate(prices, highs, lows, opens, volumes, **params):\n"
            "    \"\"\"\n"
            "    璁＄畻鑷畾涔夋寚鏍嘰n"
            "    \n"
            "    鍙傛暟:\n"
            "        prices: 鏀剁洏浠锋暟缁刓n"
            "        highs: 鏈€楂樹环鏁扮粍\n"
            "        lows: 鏈€浣庝环鏁扮粍\n"
            "        opens: 寮€鐩樹环鏁扮粍\n"
            "        volumes: 鎴愪氦閲忔暟缁刓n"
            "        **params: 鑷畾涔夊弬鏁癨n"
            "    \n"
            "    杩斿洖:\n"
            "        dict: {\n"
            "            'name': '鎸囨爣鍚嶇О',\n"
            "            'values': [鎸囨爣鍊兼暟缁刔,\n"
            "            'color': '#3B82F6',\n"
            "            'lineWidth': 1,\n"
            "            'lineStyle': 'solid',\n"
            "            'visible': True\n"
            "        }\n"
            "    \"\"\"\n"
            "    # 绀轰緥锛氱畝鍗曠Щ鍔ㄥ钩鍧囩嚎\n"
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
            "        'color': '#3B82F6',\n"
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
            "name: 鎴戠殑鑷畾涔夌瓥鐣n"
            "version: 1.0.0\n"
            "author: WealthPilot User\n"
            "description: 鑷畾涔変氦鏄撶瓥鐣n"
            "type: strategy\n"
            "\"\"\"\n"
            "\n"
            "def evaluate(prices, highs, lows, opens, volumes, **params):\n"
            "    \"\"\"\n"
            "    璇勪及绛栫暐淇″彿\n"
            "    \n"
            "    杩斿洖:\n"
            "        dict: {\n"
            "            'signal': 'buy' | 'sell' | 'hold',\n"
            "            'strength': 0-100,\n"
            "            'reason': '淇″彿鍘熷洜'\n"
            "        }\n"
            "    \"\"\"\n"
            "    # 绀轰緥锛氱畝鍗曞潎绾跨瓥鐣n"
            "    if len(prices) < 20:\n"
            "        return {'signal': 'hold', 'strength': 0, 'reason': '鏁版嵁涓嶈冻'}\n"
            "    \n"
            "    ma5 = sum(prices[-5:]) / 5\n"
            "    ma20 = sum(prices[-20:]) / 20\n"
            "    \n"
            "    if ma5 > ma20:\n"
            "        return {'signal': 'buy', 'strength': 70, 'reason': 'MA5涓婄┛MA20'}\n"
            "    elif ma5 < ma20:\n"
            "        return {'signal': 'sell', 'strength': 70, 'reason': 'MA5涓嬬┛MA20'}\n"
            "    else:\n"
            "        return {'signal': 'hold', 'strength': 30, 'reason': '鏃犳槑纭俊鍙?}\n"
        );
    } else if (type == PluginType::Alert) {
        template_ = QStringLiteral(
            "#!/usr/bin/env python3\n"
            "# -*- coding: utf-8 -*-\n"
            "\"\"\"\n"
            "@plugin\n"
            "name: 鎴戠殑鑷畾涔夐璀n"
            "version: 1.0.0\n"
            "author: WealthPilot User\n"
            "description: 鑷畾涔夐璀︽潯浠禱n"
            "type: alert\n"
            "\"\"\"\n"
            "\n"
            "def check(prices, highs, lows, opens, volumes, **params):\n"
            "    \"\"\"\n"
            "    妫€鏌ラ璀︽潯浠禱n"
            "    \n"
            "    杩斿洖:\n"
            "        dict: {\n"
            "            'triggered': True/False,\n"
            "            'message': '棰勮娑堟伅',\n"
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
            "        direction = '涓婃定' if change > 0 else '涓嬭穼'\n"
            "        return {\n"
            "            'triggered': True,\n"
            "            'message': f'鑲′环{direction}{abs(change):.2f}%',\n"
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

    // 鏋勫缓JSON杈撳叆
    QJsonObject inputObj;
    for (auto it = params.begin(); it != params.end(); ++it) {
        inputObj[it.key()] = QJsonValue::fromVariant(it.value());
    }

    QString inputJson = QString::fromUtf8(QJsonDocument(inputObj).toJson());

    // 鎵цPython鑴氭湰
    process.start(m_pythonPath, QStringList() << QStringLiteral("-c") << script);

    if (!process.waitForStarted(5000)) {
        LOG_ERROR("Failed to start Python process");
        return false;
    }

    // 鍙戦€佽緭鍏ユ暟鎹?    process.write(inputJson.toUtf8());
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

    // 瑙ｆ瀽杈撳嚭
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

    // 鍖呰鑴氭湰锛屾坊鍔犳暟鎹緭鍏?    QString wrapper = QStringLiteral(
        "import json\n"
        "import sys\n"
        "\n"
        "def main():\n"
        "    # 璇诲彇杈撳叆鏁版嵁\n"
        "    input_data = json.loads(sys.stdin.read())\n"
        "    params = input_data.get('params', {})\n"
        "\n"
        "    # 鎻愪緵浠锋牸鏁版嵁\n"
        "    prices = %1\n"
        "    highs = %2\n"
        "    lows = %3\n"
        "    opens = %4\n"
        "    volumes = %5\n"
        "\n"
        "    # 鎵ц鐢ㄦ埛鑴氭湰\n"
        "    result = calculate(prices, highs, lows, opens, volumes, **params)\n"
        "\n"
        "    # 杈撳嚭缁撴灉\n"
        "    print(json.dumps(result))\n"
        "\n"
        "%6\n"
        "\n"
        "main()\n"
    );

    // 杞崲鏁版嵁涓篜ython鍒楄〃
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

    // 瑙ｆ瀽鏂囨。瀛楃涓蹭腑鐨勫厓鏁版嵁
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
