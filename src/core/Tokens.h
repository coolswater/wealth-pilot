/**
 * @file Tokens.h
 * @brief 设计令牌系统 - WealthPilot UI 设计规范
 *
 * @details 基于 WealthPilot UI 设计规范文档
 * 定义颜色、间距、字体、圆角等设计变量
 * 所有 UI 组件应使用这些令牌，而非硬编码值
 */

#ifndef TOKENS_H
#define TOKENS_H

#include <QString>
#include <QColor>
#include <QFont>

/**
 * @brief 设计令牌命名空间
 *
 * 基于 WealthPilot UI 设计规范：
 * - 深色主题专业金融应用
 * - "稳健、智能、可信" 设计理念
 * - 一致的视觉语言
 */
namespace Tokens {

// ==================== 颜色系统 ====================
// 基于 WealthPilot UI 设计规范

namespace Colors {
// ========== 主色调 ==========
inline const QString Primary = "#3B82F6";        ///< 主色（蓝色）- 品牌标识
inline const QString PrimaryHover = "#2563EB";  ///< 主色悬停
inline const QString PrimaryLight = "#60A5FA";  ///< 浅主色
inline const QString PrimaryDark = "#1D4ED8";   ///< 深主色

// ========== 辅助色 ==========
inline const QString Secondary = "#6366F1";     ///< 辅助色（紫色）
inline const QString SecondaryLight = "#818CF8";

// ========== 功能色 ==========
inline const QString Success = "#10B981";       ///< 成功/上涨
inline const QString SuccessLight = "#34D399";
inline const QString SuccessBg = "rgba(16, 185, 129, 0.1)";

inline const QString Danger = "#EF4444";        ///< 错误/下跌
inline const QString DangerLight = "#F87171";
inline const QString DangerBg = "rgba(239, 68, 68, 0.1)";

inline const QString Warning = "#F97316";       ///< 警告
inline const QString WarningLight = "#FB923C";
inline const QString WarningBg = "rgba(249, 115, 22, 0.1)";

inline const QString Info = "#0EA5E9";          ///< 信息
inline const QString InfoLight = "#38BDF8";
inline const QString InfoBg = "rgba(14, 165, 233, 0.1)";

// ========== 背景色 ==========
inline const QString BgBase = "#1A1F2E";        ///< 主背景
inline const QString BgSurface = "#0F1419";     ///< 表面色（侧边栏等）
inline const QString BgElevated = "#242937";    ///< 抬升色（卡片、弹窗）
inline const QString BgHover = "rgba(255, 255, 255, 0.05)";
inline const QString BgActive = "rgba(59, 130, 246, 0.15)";
inline const QString BgOverlay = "rgba(0, 0, 0, 0.5)";
inline const QString BgCard = "rgba(255, 255, 255, 0.03)";

// ========== 文字色 ==========
inline const QString TextPrimary = "#FFFFFF";      ///< 主文字
inline const QString TextSecondary = "#9CA3AF";    ///< 次要文字
inline const QString TextTertiary = "#6B7280";     ///< 第三级文字
inline const QString TextDisabled = "#4B5563";     ///< 禁用文字
inline const QString TextInverse = "#1A1F2E";      ///< 反色文字

// ========== 边框色 ==========
inline const QString Border = "rgba(255, 255, 255, 0.08)";
inline const QString BorderLight = "rgba(255, 255, 255, 0.05)";
inline const QString BorderHover = "rgba(255, 255, 255, 0.15)";
inline const QString BorderFocus = "#3B82F6";

// ========== 图表色系 ==========
inline const QString ChartBlue = "#3B82F6";
inline const QString ChartPurple = "#8B5CF6";
inline const QString ChartGreen = "#10B981";
inline const QString ChartYellow = "#F59E0B";
inline const QString ChartRed = "#EF4444";
inline const QString ChartCyan = "#06B6D4";
inline const QString ChartOrange = "#F97316";
inline const QString ChartGray = "#6B7280";

// ========== 渐变 ==========
inline const QString GradientPrimary = "linear-gradient(135deg, #3B82F6 0%, #6366F1 100%)";
inline const QString GradientSurface = "linear-gradient(135deg, #1A1F2E 0%, #0F1419 100%)";
}

// QColor 版本（用于代码中）
namespace Color {
inline QColor primary() { return QColor(Colors::Primary); }
inline QColor success() { return QColor(Colors::Success); }
inline QColor danger() { return QColor(Colors::Danger); }
inline QColor warning() { return QColor(Colors::Warning); }
inline QColor bgBase() { return QColor(Colors::BgBase); }
inline QColor bgSurface() { return QColor(Colors::BgSurface); }
inline QColor bgElevated() { return QColor(Colors::BgElevated); }
inline QColor textPrimary() { return QColor(Colors::TextPrimary); }
inline QColor textSecondary() { return QColor(Colors::TextSecondary); }
}

// ==================== 间距系统 ====================
// 基于 WealthPilot UI 设计规范

namespace Spacing {
constexpr int XS = 4;       ///< 极小间距 - 图标与文字间距
constexpr int SM = 8;       ///< 小间距 - 紧凑元素间距
constexpr int MD = 16;      ///< 中间距 - 标准元素间距
constexpr int LG = 24;      ///< 大间距 - 区块间距
constexpr int XL = 32;      ///< 超大间距 - 大区块间距
constexpr int XXL = 48;     ///< 特大间距 - 页面区块间距

// 别名
constexpr int None = 0;
constexpr int Tight = XS;
constexpr int Compact = SM;
constexpr int Normal = MD;
constexpr int Relaxed = LG;
constexpr int Loose = XL;
}

// ==================== 圆角系统 ====================

namespace Radius {
constexpr int SM = 4;       ///< 小圆角 - 标签、徽章
constexpr int MD = 8;       ///< 中圆角 - 按钮、输入框
constexpr int LG = 12;      ///< 大圆角 - 卡片
constexpr int XL = 16;      ///< 超大圆角 - 弹窗、面板
constexpr int XXL = 20;     ///< 特大圆角 - 头部区域
constexpr int Full = 9999;  ///< 完全圆形 - 头像
constexpr int None = 0;
}

// ==================== 字体系统 ====================
// 基于 WealthPilot UI 设计规范

namespace Font {
// 字体族
inline const QString Family = "'Roboto', 'Microsoft YaHei', 'PingFang SC', sans-serif";
inline const QString Mono = "'Roboto Mono', 'Consolas', monospace";

// 字号 (基于设计规范)
namespace Size {
constexpr int H1 = 32;      ///< 页面标题
constexpr int H2 = 24;      ///< 区块标题
constexpr int H3 = 20;      ///< 卡片标题
constexpr int Body = 14;    ///< 正文文本
constexpr int Small = 12;   ///< 辅助文本、说明
constexpr int Data = 16;    ///< 数据展示
constexpr int DataLarge = 24; ///< 大数据
constexpr int DataXLarge = 32; ///< 超大数据（账户余额等）
}

// 字重
namespace Weight {
constexpr int Normal = 400;
constexpr int Medium = 500;
constexpr int SemiBold = 600;
constexpr int Bold = 700;
}

// 行高
constexpr double LineHeightTight = 1.25;
constexpr double LineHeightNormal = 1.5;
constexpr double LineHeightRelaxed = 1.75;
}

// ==================== 阴影系统 ====================

namespace Shadow {
inline const QString SM = "0 2px 8px rgba(0, 0, 0, 0.15)";
inline const QString MD = "0 4px 16px rgba(0, 0, 0, 0.2)";
inline const QString LG = "0 8px 32px rgba(0, 0, 0, 0.25)";
inline const QString XL = "0 16px 48px rgba(0, 0, 0, 0.3)";
inline const QString Card = "0 10px 40px rgba(0, 0, 0, 0.4)";
inline const QString Hover = "0 20px 60px rgba(0, 0, 0, 0.5)";
}

// ==================== 动画系统 ====================

namespace Animation {
// 时长
constexpr int DurationFast = 150;      ///< 快速动画 - 悬停
constexpr int DurationNormal = 250;    ///< 正常动画 - 页面切换
constexpr int DurationSlow = 400;      ///< 慢速动画 - 弹窗
constexpr int DurationSlower = 600;    ///< 更慢动画 - 特效

// 缓动曲线
inline const QString EaseOut = "ease-out";
inline const QString EaseIn = "ease-in";
inline const QString EaseInOut = "ease-in-out";
inline const QString EaseOutCubic = "cubic-bezier(0.33, 1, 0.68, 1)";
inline const QString EaseOutBack = "cubic-bezier(0.34, 1.56, 0.64, 1)";
}

// ==================== 尺寸系统 ====================

namespace Size {
// 侧边栏
constexpr int SidebarExpanded = 150;
constexpr int SidebarCollapsed = 64;

// AI 面板
constexpr int AIPanelWidth = 280;
constexpr int AIPanelCollapsed = 48;

// 头像
constexpr int AvatarSM = 24;
constexpr int AvatarMD = 32;
constexpr int AvatarLG = 40;
constexpr int AvatarXL = 56;
constexpr int AvatarXXL = 80;

// 按钮
constexpr int ButtonHeightSM = 28;
constexpr int ButtonHeightMD = 36;
constexpr int ButtonHeightLG = 44;
constexpr int ButtonHeightXL = 52;

// 图标
constexpr int IconSM = 16;
constexpr int IconMD = 20;
constexpr int IconLG = 24;
constexpr int IconXL = 32;
constexpr int IconXXL = 48;

// 输入框
constexpr int InputHeightSM = 32;
constexpr int InputHeightMD = 40;
constexpr int InputHeightLG = 48;

// 卡片
constexpr int CardMinHeight = 100;
constexpr int StatCardHeight = 140;
constexpr int CardMinWidth = 200;

// 窗口
constexpr int WindowMinWidth = 1280;
constexpr int WindowMinHeight = 800;
constexpr int WindowDefaultWidth = 1600;
constexpr int WindowDefaultHeight = 900;

// 顶部导航
constexpr int TopNavHeight = 56;

// 表格行高
constexpr int TableRowHeight = 48;
constexpr int TableRowHeightCompact = 36;
}

// ==================== 断点系统（响应式）====================

namespace Breakpoint {
constexpr int SM = 640;    ///< 手机横屏
constexpr int MD = 768;    ///< 平板竖屏
constexpr int LG = 1024;   ///< 平板横屏/小桌面
constexpr int XL = 1280;   ///< 桌面
constexpr int XXL = 1536;  ///< 大桌面
constexpr int XXXL = 1920; // 超大桌面
}

// ==================== Z-Index 层级 ====================

namespace ZIndex {
constexpr int Base = 0;
constexpr int Dropdown = 100;
constexpr int Sticky = 200;
constexpr int Fixed = 300;
constexpr int ModalBackdrop = 400;
constexpr int Modal = 500;
constexpr int Popover = 600;
constexpr int Tooltip = 700;
constexpr int Toast = 800;
constexpr int Top = 900;
}

// ==================== 辅助函数 ====================

/**
 * @brief 根据涨跌获取颜色
 */
inline QString getTrendColor(double change) {
    if (change > 0.0001) return Colors::Success;
    if (change < -0.0001) return Colors::Danger;
    return Colors::TextSecondary;
}

/**
 * @brief 根据涨跌获取箭头
 */
inline QString getTrendArrow(double change) {
    if (change > 0.0001) return "▲";
    if (change < -0.0001) return "▼";
    return "—";
}

/**
 * @brief 格式化涨跌幅
 */
inline QString formatChangePercent(double change, double base) {
    if (base == 0) return "0.00%";
    double percent = (change / base) * 100;
    QString sign = percent >= 0 ? "+" : "";
    return QString("%1%2%").arg(sign).arg(percent, 0, 'f', 2);
}

/**
 * @brief 格式化金额
 */
inline QString formatMoney(double amount, int decimals = 2) {
    QString sign = amount >= 0 ? "" : "-";
    double absAmount = qAbs(amount);

    if (absAmount >= 100000000) {
        return QString("%1¥%2亿").arg(sign).arg(absAmount / 100000000, 0, 'f', decimals);
    } else if (absAmount >= 10000) {
        return QString("%1¥%2万").arg(sign).arg(absAmount / 10000, 0, 'f', decimals);
    } else {
        return QString("%1¥%2").arg(sign).arg(absAmount, 0, 'f', decimals);
    }
}

/**
 * @brief 格式化成交量
 */
inline QString formatVolume(double volume) {
    if (volume >= 100000000) {
        return QString("%1亿").arg(volume / 100000000, 0, 'f', 2);
    } else if (volume >= 10000) {
        return QString("%1万").arg(volume / 10000, 0, 'f', 2);
    } else {
        return QString::number(static_cast<int>(volume));
    }
}

} // namespace Tokens

#endif // TOKENS_H
