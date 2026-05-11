/**
 * StockCard.qml - 股票卡片组件
 * 
 * 特点：
 * - 流畅的数字变化动画
 * - 涨跌颜色自动切换
 * - 悬停效果
 */
import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    
    // 公开属性
    property string symbol: ""
    property string name: ""
    property real price: 0
    property real change: 0
    property real changePercent: 0
    property int volume: 0
    
    // 主题颜色
    property color upColor: "#EF4444"
    property color downColor: "#10B981"
    property color flatColor: "#9CA3AF"
    property color bgColor: "#1A2332"
    property color hoverColor: "#2D3748"
    property color textColor: "#F3F4F6"
    property color subTextColor: "#9CA3AF"
    
    // 内部状态
    property bool isHovered: false
    property bool isUp: changePercent > 0
    property bool isDown: changePercent < 0
    
    // 信号
    signal clicked()
    
    width: 200
    height: 80
    radius: 8
    color: isHovered ? hoverColor : bgColor
    
    // 悬停动画
    Behavior on color {
        ColorAnimation { duration: 150 }
    }
    
    // 价格变化动画
    Behavior on price {
        NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
    }
    
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        
        onEntered: isHovered = true
        onExited: isHovered = false
        onClicked: root.clicked()
    }
    
    Column {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 6
        
        // 股票代码和名称
        Row {
            spacing: 8
            
            Text {
                text: symbol
                color: textColor
                font.pixelSize: 14
                font.bold: true
            }
            
            Text {
                text: name
                color: subTextColor
                font.pixelSize: 12
            }
        }
        
        // 价格
        Text {
            text: price.toFixed(2)
            color: isUp ? upColor : (isDown ? downColor : flatColor)
            font.pixelSize: 20
            font.bold: true
            
            Behavior on color {
                ColorAnimation { duration: 200 }
            }
        }
        
        // 涨跌幅和成交量
        Row {
            spacing: 16
            
            Text {
                text: (isUp ? "+" : "") + changePercent.toFixed(2) + "%"
                color: isUp ? upColor : (isDown ? downColor : flatColor)
                font.pixelSize: 12
                
                Behavior on color {
                    ColorAnimation { duration: 200 }
                }
            }
            
            Text {
                text: "量 " + formatVolume(volume)
                color: subTextColor
                font.pixelSize: 11
            }
        }
    }
    
    // 涨跌指示条
    Rectangle {
        width: 3
        height: parent.height
        anchors.left: parent.left
        anchors.top: parent.top
        radius: 1.5
        
        color: isUp ? upColor : (isDown ? downColor : flatColor)
        
        Behavior on color {
            ColorAnimation { duration: 200 }
        }
    }
    
    // 格式化成交量
    function formatVolume(vol) {
        if (vol >= 100000000) return (vol / 100000000).toFixed(2) + "亿"
        if (vol >= 10000) return (vol / 10000).toFixed(2) + "万"
        return vol.toString()
    }
}
