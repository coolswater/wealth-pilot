/**
 * WealthPilot QML 主入口
 */
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 1600
    height: 900
    title: "WealthPilot - 财富领航AI助手"
    
    // 深色主题背景
    color: "#0A0E17"
    
    // 主题配置
    property var theme: QtObject {
        readonly property color bgPrimary: "#0A0E17"
        readonly property color bgSecondary: "#111827"
        readonly property color bgElevated: "#1A2332"
        readonly property color primary: "#3B82F6"
        readonly property color danger: "#EF4444"      // 红涨
        readonly property color success: "#10B981"     // 绿跌
        readonly property color textPrimary: "#F3F4F6"
        readonly property color textSecondary: "#9CA3AF"
        readonly property color border: "#2D3748"
    }
    
    // 全局字体
    font.family: "Roboto, Microsoft YaHei, sans-serif"
    font.pixelSize: 14
}
