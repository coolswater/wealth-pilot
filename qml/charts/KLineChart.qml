/**
 * KLineChart.qml - K线图表组件
 * 
 * 特点：
 * - GPU 加速渲染
 * - 流畅的缩放和平移动画
 * - 支持手势交互
 * - 自动数据绑定
 */
import QtQuick 2.15
import QtQuick.Controls 2.15
import QtCharts 2.15

Item {
    id: root
    
    // 公开属性
    property var model: null          // K线数据模型
    property int visibleCount: 100    // 可见K线数量
    property int minVisibleCount: 20  // 最小可见数量
    property int maxVisibleCount: 500 // 最大可见数量
    
    // 主题颜色
    property color upColor: "#EF4444"      // 红涨
    property color downColor: "#10B981"    // 绿跌
    property color gridColor: "#2D3748"
    property color textColor: "#9CA3AF"
    property color highlightColor: "#3B82F6"
    
    // 内部状态
    property int startIndex: 0
    property real scale: 1.0
    
    // 信号
    signal candleClicked(int index)
    signal crosshairMoved(int index, real price)
    
    ChartView {
        id: chartView
        anchors.fill: parent
        antialiasing: true
        backgroundColor: "transparent"
        
        // 隐藏图例
        legend.visible: false
        
        // 动画配置
        animationOptions: ChartView.SeriesAnimations
        animationDuration: 200
        
        // X轴（时间）
        DateTimeAxis {
            id: axisX
            format: "MM-dd"
            tickCount: 6
            labelsColor: textColor
            gridLineColor: gridColor
            minorGridLineColor: Qt.rgba(gridColor.r, gridColor.g, gridColor.b, 0.3)
        }
        
        // Y轴（价格）
        ValueAxis {
            id: axisY
            tickCount: 5
            labelsColor: textColor
            gridLineColor: gridColor
            minorGridLineColor: Qt.rgba(gridColor.r, gridColor.g, gridColor.b, 0.3)
            labelFormat: "%.2f"
        }
    
        // K线系列
        CandlestickSeries {
            id: candlestickSeries
            axisX: axisX
            axisY: axisY
            
            increasingColor: upColor
            decreasingColor: downColor
            
            // 点击事件
            onClicked: function(point) {
                root.candleClicked(point.index)
            }
            
            // 悬停事件
            onHovered: function(point, state) {
                if (state) {
                    root.crosshairMoved(point.index, point.close)
                }
            }
            
            Behavior on opacity {
                NumberAnimation { duration: 150 }
            }
        }
        
        // 成交量系列
        AreaSeries {
            id: volumeSeries
            axisX: axisX
            axisYRight: axisYVolume
            
            color: "#3B82F6"
            borderWidth: 0
            opacity: 0.3
        }
        
        // 成交量Y轴
        ValueAxis {
            id: axisYVolume
            visible: false
        }
        
        // MA均线
        LineSeries {
            id: ma5Line
            color: "#F59E0B"
            width: 1
            opacity: 0.8
        }
        
        LineSeries {
            id: ma10Line
            color: "#8B5CF6"
            width: 1
            opacity: 0.8
        }
        
        LineSeries {
            id: ma20Line
            color: "#06B6D4"
            width: 1
            opacity: 0.8
        }
    }
    
    // 十字光标
    Canvas {
        id: crosshairCanvas
        anchors.fill: parent
        visible: false
        
        property real crossX: 0
        property real crossY: 0
        
        onPaint: {
            var ctx = getContext('2d')
            ctx.clearRect(0, 0, width, height)
            
            ctx.strokeStyle = highlightColor
            ctx.lineWidth = 1
            ctx.setLineDash([5, 5])
            
            // 垂直线
            ctx.beginPath()
            ctx.moveTo(crossX, 0)
            ctx.lineTo(crossX, height)
            ctx.stroke()
            
            // 水平线
            ctx.beginPath()
            ctx.moveTo(0, crossY)
            ctx.lineTo(width, crossY)
            ctx.stroke()
        }
        
        function updateCrosshair(x, y) {
            crossX = x
            crossY = y
            requestPaint()
        }
    }
    
    // 信息提示框
    Rectangle {
        id: infoBox
        visible: crosshairCanvas.visible
        width: 180
        height: 150
        color: Qt.rgba(26, 35, 50, 0.95)
        border.color: highlightColor
        border.width: 1
        radius: 8
        
        property var candleData: null
        
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 5
            
            Text {
                text: infoBox.candleData ? Qt.formatDate(infoBox.candleData.time, "yyyy-MM-dd") : ""
                color: textColor
                font.pixelSize: 12
            }
            Text {
                text: infoBox.candleData ? "开: " + infoBox.candleData.open.toFixed(2) : ""
                color: textColor
                font.pixelSize: 12
            }
            Text {
                text: infoBox.candleData ? "高: " + infoBox.candleData.high.toFixed(2) : ""
                color: upColor
                font.pixelSize: 12
            }
            Text {
                text: infoBox.candleData ? "低: " + infoBox.candleData.low.toFixed(2) : ""
                color: downColor
                font.pixelSize: 12
            }
            Text {
                text: infoBox.candleData ? "收: " + infoBox.candleData.close.toFixed(2) : ""
                color: infoBox.candleData && infoBox.candleData.close >= infoBox.candleData.open ? upColor : downColor
                font.pixelSize: 12
                font.bold: true
            }
            Text {
                text: infoBox.candleData ? "量: " + formatVolume(infoBox.candleData.volume) : ""
                color: textColor
                font.pixelSize: 12
            }
        }
    }
    
    // 鼠标交互
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        
        property real lastX: 0
        property bool isDragging: false
        
        onPositionChanged: function(mouse) {
            if (isDragging) {
                var dx = mouse.x - lastX
                var newStart = startIndex - Math.round(dx / candleWidth)
                startIndex = Math.max(0, Math.min(newStart, model ? model.count - visibleCount : 0))
            } else {
                crosshairCanvas.visible = true
                crosshairCanvas.updateCrosshair(mouse.x, mouse.y)
                
                // 更新信息框位置
                infoBox.x = Math.min(mouse.x + 15, root.width - infoBox.width - 10)
                infoBox.y = Math.max(mouse.y - infoBox.height - 10, 10)
            }
            lastX = mouse.x
        }
        
        onPressed: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                isDragging = true
                lastX = mouse.x
            }
        }
        
        onReleased: {
            isDragging = false
        }
        
        onExited: {
            crosshairCanvas.visible = false
        }
        
        onWheel: function(wheel) {
            // 缩放
            var delta = wheel.angleDelta.y / 120
            var newCount = visibleCount - delta * 10
            visibleCount = Math.max(minVisibleCount, Math.min(maxVisibleCount, newCount))
        }
    }
    
    // 数据更新函数
    function updateData() {
        if (!model || model.count === 0) return
        
        candlestickSeries.clear()
        ma5Line.clear()
        ma10Line.clear()
        ma20Line.clear()
        
        var minPrice = Infinity
        var maxPrice = -Infinity
        var minTime = Infinity
        var maxTime = -Infinity
        
        var endIndex = Math.min(startIndex + visibleCount, model.count)
        
        for (var i = startIndex; i < endIndex; i++) {
            var candle = model.get(i)
            if (!candle) continue
            
            candlestickSeries.append(candle.timestamp, candle.open, candle.high, candle.low, candle.close)
            
            minPrice = Math.min(minPrice, candle.low)
            maxPrice = Math.max(maxPrice, candle.high)
            minTime = Math.min(minTime, candle.timestamp)
            maxTime = Math.max(maxTime, candle.timestamp)
            
            // MA线
            if (candle.ma5) ma5Line.append(candle.timestamp, candle.ma5)
            if (candle.ma10) ma10Line.append(candle.timestamp, candle.ma10)
            if (candle.ma20) ma20Line.append(candle.timestamp, candle.ma20)
        }
        
        // 更新轴范围
        axisX.min = new Date(minTime)
        axisX.max = new Date(maxTime)
        
        var priceMargin = (maxPrice - minPrice) * 0.1
        axisY.min = minPrice - priceMargin
        axisY.max = maxPrice + priceMargin
    }
    
    // 格式化成交量
    function formatVolume(vol) {
        if (vol >= 100000000) return (vol / 100000000).toFixed(2) + "亿"
        if (vol >= 10000) return (vol / 10000).toFixed(2) + "万"
        return vol.toString()
    }
    
    // 计算单根K线宽度
    property real candleWidth: chartView.plotArea.width / visibleCount
    
    // 监听模型变化
    onModelChanged: updateData()
    onVisibleCountChanged: updateData()
    onStartIndexChanged: updateData()
    
    // 平滑动画
    Behavior on visibleCount {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }
}
