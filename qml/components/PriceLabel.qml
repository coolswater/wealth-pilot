/**
 * PriceLabel.qml - 价格标签组件
 */
import QtQuick 2.15
import QtQuick.Controls 2.15

Text {
    id: root
    
    property real value: 0
    property bool showSign: false
    property int decimals: 2
    
    // 主题颜色
    property color upColor: "#EF4444"
    property color downColor: "#10B981"
    property color flatColor: "#9CA3AF"
    
    // 自动判断颜色
    color: value > 0 ? upColor : (value < 0 ? downColor : flatColor)
    
    text: {
        var formatted = Math.abs(value).toFixed(decimals)
        if (showSign && value !== 0) {
            return (value > 0 ? "+" : "-") + formatted
        }
        return formatted
    }
    
    // 平滑颜色变化
    Behavior on color {
        ColorAnimation { duration: 200 }
    }
    
    // 数字变化动画
    Behavior on value {
        NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
    }
}
