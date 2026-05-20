/**
 * @file ErrorHandler.cpp
 * @brief 统一错误处理助手类实现
 */

#include "ErrorHandler.h"
#include "../../utils/Logger.h"
#include <QMessageBox>

namespace WealthPilot {

ErrorHandler* ErrorHandler::instance()
{
    static ErrorHandler* inst = new ErrorHandler();
    return inst;
}

ErrorHandler::ErrorHandler(QObject* parent)
    : QObject(parent)
{
}

void ErrorHandler::handleError(const ErrorInfo& error, bool showUser)
{
    if (error.isOk()) return;

    // 记录日志
    LOG_ERROR(QString("[%1] %2").arg(error.codeName(), error.message));
    if (!error.detail.isEmpty()) {
        LOG_ERROR(QString("Detail: %1").arg(error.detail));
    }

    // 发送信号
    emit errorOccurred(error);

    // 调用回调
    if (m_errorCallback) {
        m_errorCallback(error);
    }

    // 显示给用户
    if (showUser) {
        QString title = QStringLiteral("错误");
        QString message = getUserMessage(error);
        QString suggestion = getRecoverySuggestion(error);
        emit showUserError(title, message, suggestion);
    }
}

void ErrorHandler::handleError(ErrorCode code, const QString& message, bool showUser)
{
    handleError(ErrorInfo(code, message), showUser);
}

QString ErrorHandler::getUserMessage(const ErrorInfo& error) const
{
    // 根据错误码返回用户友好的消息
    int code = error.codeValue();

    if (code >= 1000 && code < 2000) {
        // 通用错误
        switch (error.code) {
            case ErrorCode::InvalidArgument:
                return QStringLiteral("参数无效，请检查输入。");
            case ErrorCode::Timeout:
                return QStringLiteral("操作超时，请稍后重试。");
            case ErrorCode::NotInitialized:
                return QStringLiteral("系统未初始化，请重启应用。");
            default:
                return error.message.isEmpty() ? QStringLiteral("操作失败。") : error.message;
        }
    }
    else if (code >= 2000 && code < 3000) {
        // 网络错误
        switch (error.code) {
            case ErrorCode::NetworkTimeout:
                return QStringLiteral("网络连接超时，请检查网络后重试。");
            case ErrorCode::NetworkDisconnected:
                return QStringLiteral("网络连接已断开，请检查网络设置。");
            case ErrorCode::NetworkServerError:
                return QStringLiteral("服务器错误，请稍后重试。");
            case ErrorCode::NetworkUnauthorized:
                return QStringLiteral("未授权访问，请检查登录状态。");
            case ErrorCode::NetworkRateLimited:
                return QStringLiteral("请求过于频繁，请稍后重试。");
            default:
                return QStringLiteral("网络错误，请检查网络连接。");
        }
    }
    else if (code >= 3000 && code < 4000) {
        // 数据库错误
        switch (error.code) {
            case ErrorCode::DatabaseOpenFailed:
                return QStringLiteral("无法打开数据库，请检查文件权限。");
            case ErrorCode::DatabaseQueryFailed:
                return QStringLiteral("数据查询失败，请重试。");
            case ErrorCode::DatabaseConnectionFailed:
                return QStringLiteral("数据库连接失败，请检查配置。");
            default:
                return QStringLiteral("数据库错误，请重试或重启应用。");
        }
    }
    else if (code >= 4000 && code < 5000) {
        // CTP错误
        switch (error.code) {
            case ErrorCode::CtpConnectFailed:
                return QStringLiteral("CTP连接失败，请检查网络和配置。");
            case ErrorCode::CtpLoginFailed:
                return QStringLiteral("CTP登录失败，请检查账号密码。");
            case ErrorCode::CtpSubscribeFailed:
                return QStringLiteral("行情订阅失败，请检查合约代码。");
            case ErrorCode::CtpOrderFailed:
                return QStringLiteral("下单失败，请检查委托参数。");
            case ErrorCode::CtpNotConnected:
                return QStringLiteral("CTP未连接，请先连接交易服务器。");
            default:
                return QStringLiteral("CTP交易错误，请检查连接状态。");
        }
    }
    else if (code >= 5000 && code < 6000) {
        // AI错误
        switch (error.code) {
            case ErrorCode::AiRequestFailed:
                return QStringLiteral("AI请求失败，请稍后重试。");
            case ErrorCode::AiRateLimited:
                return QStringLiteral("AI请求过于频繁，请稍后重试。");
            case ErrorCode::AiModelNotAvailable:
                return QStringLiteral("AI模型不可用，请检查配置。");
            case ErrorCode::AiContextTooLong:
                return QStringLiteral("对话内容过长，请精简后重试。");
            default:
                return QStringLiteral("AI服务错误，请稍后重试。");
        }
    }
    else if (code >= 6000 && code < 7000) {
        // 配置错误
        switch (error.code) {
            case ErrorCode::ConfigFileNotFound:
                return QStringLiteral("配置文件未找到，将使用默认配置。");
            case ErrorCode::ConfigParseError:
                return QStringLiteral("配置文件格式错误，请检查配置。");
            case ErrorCode::ConfigInvalidValue:
                return QStringLiteral("配置值无效，已使用默认值。");
            default:
                return QStringLiteral("配置错误，请检查设置。");
        }
    }
    else if (code >= 7000 && code < 8000) {
        // 缓存错误
        return QStringLiteral("缓存错误，数据可能需要重新加载。");
    }
    else if (code >= 8000 && code < 9000) {
        // 插件错误
        switch (error.code) {
            case ErrorCode::PluginLoadFailed:
                return QStringLiteral("插件加载失败，请检查插件文件。");
            case ErrorCode::PluginNotFound:
                return QStringLiteral("插件未找到，请检查安装。");
            case ErrorCode::PluginVersionMismatch:
                return QStringLiteral("插件版本不匹配，请更新插件。");
            default:
                return QStringLiteral("插件错误，请检查插件状态。");
        }
    }

    return error.message.isEmpty() ? QStringLiteral("未知错误。") : error.message;
}

QString ErrorHandler::getRecoverySuggestion(const ErrorInfo& error) const
{
    int code = error.codeValue();

    if (code >= 2000 && code < 3000) {
        return QStringLiteral("建议：检查网络连接，确认服务器地址正确，稍后重试。");
    }
    else if (code >= 3000 && code < 4000) {
        return QStringLiteral("建议：检查数据库文件是否存在，确认有读写权限，重启应用。");
    }
    else if (code >= 4000 && code < 5000) {
        return QStringLiteral("建议：检查CTP配置，确认账号密码正确，检查网络连接。");
    }
    else if (code >= 5000 && code < 6000) {
        return QStringLiteral("建议：检查AI服务配置，确认API密钥有效，稍后重试。");
    }
    else if (code >= 6000 && code < 7000) {
        return QStringLiteral("建议：检查配置文件格式，恢复默认配置或重新设置。");
    }

    return QString();
}

void ErrorHandler::setErrorCallback(std::function<void(const ErrorInfo&)> callback)
{
    m_errorCallback = std::move(callback);
}

} // namespace WealthPilot
