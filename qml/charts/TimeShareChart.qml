/**
 * TimeShareChart.qml - 分时图组件
 * 
 * 特点：
 * - 60fps 流畅刷新
 * - GPU 加速渲染
 * - 平滑的价格变化动画
 */
import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    
    // 公开属性
    property var model: null           // 分时数据模型
    property double basePrice: 0       // 昨收价
    property int updateInterval: 16    // 更新间隔（毫秒），默认60fps
    
    // 主题颜色
    property color lineColor: "#3B82F6"
    property color avgColor: "#F59E0B"
    property color upFill: Qt.rgba(239, 68, 68, 0.3)     // 红涨填充
    property color downFill: Qt.rgba(16, 185, 129, 0.3)   // 绿跌填充
    property color gridColor: "#2D3748"
    property color textColor: "#9CA3AF"
    property color baseLineColor: "#6B7280"
    
    // 内部状态
    property real minPrice: 0
    property real maxPrice: 0
    property real priceRange: 0
    property var pricePoints: []
    property var volumePoints: []
    
    // 信号
    signal pointClicked(int index)
    signal crosshairMoved(int index, real price)
    
    // 主图表区域
    Canvas {
        id: priceCanvas
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: parent.height * 0.7
        
        onPaint: {
            var ctx = getContext('2d')
            ctx.clearRect(0, 0, width, height)
            
            if (pricePoints.length < 2) return
            
            // 计算坐标
            var chartWidth = width - 60
            var chartHeight = height - 20
            var startX = 50
            var startY = 10
            
            // 绘制网格
            drawGrid(ctx, startX, startY, chartWidth, chartHeight)
            
            // 绘制基准线（昨收价）
            drawBaseLine(ctx, startX, startY, chartWidth, chartHeight)
            
            // 绘制价格区域填充
            drawPriceArea(ctx, startX, startY, chartWidth, chartHeight)
            
            // 绘制价格线
            drawPriceLine(ctx, startX, startY, chartWidth, chartHeight)
            
            // 绘制均线
            drawAvgLine(ctx, startX, startY, chartWidth, chartHeight)
            
            // 绘制价格标签
            drawPriceLabels(ctx, startX, startY, chartHeight)
        }
        
        function drawGrid(ctx, startX, startY, chartWidth, chartHeight) {
            ctx.strokeStyle = gridColor
            ctx.lineWidth = 0.5
            
            // 横线
            for (var i = 0; i <= 4; i++) {
                var y = startY + i * chartHeight / 4
                ctx.beginPath()
                ctx.moveTo(startX, y)
                ctx.lineTo(startX + chartWidth, y)
                ctx.stroke()
            }
            
            // 竖线
            for (var j = 0; j <= 4; j++) {
                var x = startX + j * chartWidth / 4
                ctx.beginPath()
                ctx.moveTo(x, startY)
                ctx.lineTo(x, startY + chartHeight)
                ctx.stroke()
            }
        }
        
        function drawBaseLine(ctx, startX, startY, chartWidth, chartHeight) {
            if (basePrice <= 0) return
            
            var baseY = startY + chartHeight / 2
            ctx.strokeStyle = baseLineColor
            ctx.lineWidth = 1
            ctx.setLineDash([5, 5])
            ctx.beginPath()
            ctx.moveTo(startX, baseY)
            ctx.lineTo(startX + chartWidth, baseY)
            ctx.stroke()
            ctx.setLineDash([])
        }
        
        function drawPriceArea(ctx, startX, startY, chartWidth, chartHeight) {
            if (pricePoints.length < 2) return
            
            var stepX = chartWidth / (pricePoints.length - 1)
            var lastPrice = pricePoints[pricePoints.length - 1].price
            var fillColor = lastPrice >= basePrice ? upFill : downFill
            
            ctx.fillStyle = fillColor
            ctx.beginPath()
            
            for (var i = 0; i < pricePoints.length; i++) {
                var x = startX + i * stepX
                var y = startY + chartHeight - (pricePoints[i].price - minPrice) / priceRange * chartHeight
                
                if (i === 0) {
                    ctx.moveTo(x, y)
                } else {
                    ctx.lineTo(x, y)
                }
            }
            
            // 闭合路径
            var lastX = startX + (pricePoints.length - 1) * stepX
            var baseY = startY + chartHeight / 2
            ctx.lineTo(lastX, baseY)
            ctx.lineTo(startX, baseY)
            ctx.closePath()
            ctx.fill()
        }
        
        function drawPriceLine(ctx, startX, startY, chartWidth, chartHeight) {
            if (pricePoints.length < 2) return
            
            var stepX = chartWidth / (pricePoints.length - 1)
            
            ctx.strokeStyle = lineColor
            ctx.lineWidth = 2
            ctx.lineCap = 'round'
            ctx.lineJoin = 'round'
            
            ctx.beginPath()
            for (var i = 0; i < pricePoints.length; i++) {
                var x = startX + i * stepX
                var y = startY + chartHeight - (pricePoints[i].price - minPrice) / priceRange * chartHeight
                
                if (i === 0) {
                    ctx.moveTo(x, y)
                } else {
                    ctx.lineTo(x, y)
                }
            }
            ctx.stroke()
        }
        
        function drawAvgLine(ctx, startX, startY, chartWidth, chartHeight) {
            var hasAvg = false
            for (var i = 0; i < pricePoints.length; i++) {
                if (pricePoints[i].avgPrice > 0) {
                    hasAvg = true
                    break
                }
            }
            
            if (!hasAvg) return
            
            var stepX = chartWidth / (pricePoints.length - 1)
            
            ctx.strokeStyle = avgColor
            ctx.lineWidth = 1
            ctx.setLineDash([3, 3])
            
            ctx.beginPath()
            var started = false
            for (var j = 0; j < pricePoints.length; j++) {
                if (pricePoints[j].avgPrice <= 0) continue
                
                var x = startX + j * stepX
                var y = startY + chartHeight - (pricePoints[j].avgPrice - minPrice) / priceRange * chartHeight
                
                if (!started) {
                    ctx.moveTo(x, y)
                    started = true
                } else {
                    ctx.lineTo(x, y)
                }
            }
            ctx.stroke()
            ctx.setLineDash([])
        }
        
        function drawPriceLabels(ctx, startX, startY, chartHeight) {
            ctx.fillStyle = textColor
            ctx.font = '11px Roboto'
            ctx.textAlign = 'right'
            
            for (var i = 0; i <= 4; i++) {
                var price = maxPrice - i * priceRange / 4
                var y = startY + i * chartHeight / 4
                
                var text = price.toFixed(2)
                if (basePrice > 0) {
                    var change = (price - basePrice) / basePrice * 100
                    text += " (" + (change >= 0 ? "+" : "") + change.toFixed(2) + "%)"
                }
                
                ctx.fillText(text, startX - 5, y + 4)
            }
        }
    }
    
    // 成交量图表区域
    Canvas {
        id: volumeCanvas
        anchors.top: priceCanvas.bottom
        anchors.topMargin: 10
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        
        onPaint: {
            var ctx = getContext('2d')
            ctx.clearRect(0, 0, width, height)
            
            if (volumePoints.length === 0) return
            
            var chartWidth = width - 60
            var chartHeight = height - 10
            var startX = 50
            var startY = 5
            
            // 找最大成交量
            var maxVol = 0
            for (var i = 0; i < volumePoints.length; i++) {
                maxVol = Math.max(maxVol, volumePoints[i].volume)
            }
            
            if (maxVol === 0) return
            
            // 绘制成交量柱
            var barWidth = chartWidth / volumePoints.length * 0.8
            var barGap = chartWidth / volumePoints.length * 0.2
            
            for (var j = 0; j < volumePoints.length; j++) {
                var x = startX + j * (barWidth + barGap)
                var barHeight = volumePoints[j].volume / maxVol * chartHeight
                var y = startY + chartHeight - barHeight
                
                // 根据价格涨跌选择颜色
                ctx.fillStyle = volumePoints[j].price >= basePrice ? 
                    Qt.rgba(239, 68, 68, 0.8) : Qt.rgba(16, 185, 129, 0.8)
                
                ctx.fillRect(x, y, barWidth, barHeight)
            }
        }
    }
    
    // 时间标签
    Row {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 50
        anchors.right: parent.right
        anchors.rightMargin: 10
        height: 20
        
        spacing: (parent.width - 60) / 5
        
        Repeater {
            model: ["09:30", "10:30", "11:30", "14:00", "15:00"]
            
            Text {
                text: modelData
                color: textColor
                font.pixelSize: 11
            }
        }
    }
    
    // 十字光标
    Canvas {
        id: crosshair
        anchors.fill: priceCanvas
        visible: false
        
        property real crossX: 0
        property real crossY: 0
        
        onPaint: {
            var ctx = getContext('2d')
            ctx.clearRect(0, 0, width, height)
            
            ctx.strokeStyle = lineColor
            ctx.lineWidth = 1
            ctx.setLineDash([5, 5])
            
            ctx.beginPath()
            ctx.moveTo(crossX, 0)
            ctx.lineTo(crossX, height)
            ctx.stroke()
            
            ctx.beginPath()
            ctx.moveTo(0, crossY)
            ctx.lineTo(width, crossY)
            ctx.stroke()
        }
        
        function updatePosition(x, y) {
            crossX = x
            crossY = y
            requestPaint()
        }
    }
    
    // 信息提示框
    Rectangle {
        id: infoBox
        visible: crosshair.visible
        width: 150
        height: 100
        color: Qt.rgba(26, 35, 50, 0.95)
        border.color: lineColor
        border.width: 1
        radius: 6
        
        property var pointData: null
        
        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 4
            
            Text {
                text: infoBox.pointData ? "时间: " + infoBox.pointData.time : ""
                color: textColor
                font.pixelSize: 11
            }
            Text {
                text: infoBox.pointData ? "价格: " + infoBox.pointData.price.toFixed(2) : ""
                color: textColor
                font.pixelSize: 11
            }
            Text {
                text: infoBox.pointData ? "均价: " + (infoBox.pointData.avgPrice || 0).toFixed(2) : ""
                color: avgColor
                font.pixelSize: 11
            }
            Text {
                text: infoBox.pointData ? "成交量: " + formatVolume(infoBox.pointData.volume) : ""
                color: textColor
                font.pixelSize: 11
            }
        }
    }
    
    // 鼠标交互
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        
        onPositionChanged: function(mouse) {
            crosshair.visible = true
            crosshair.updatePosition(mouse.x, mouse.y)
            
            // 更新信息框
            infoBox.x = Math.min(mouse.x + 15, root.width - infoBox.width - 10)
            infoBox.y = Math.max(mouse.y - infoBox.height - 10, 10)
            
            // 计算当前点索引
            var chartWidth = priceCanvas.width - 60
            var index = Math.floor((mouse.x - 50) / chartWidth * pricePoints.length)
            index = Math.max(0, Math.min(index, pricePoints.length - 1))
            
            if (pricePoints[index]) {
                infoBox.pointData = pricePoints[index]
                root.crosshairMoved(index, pricePoints[index].price)
            }
        }
        
        onExited: {
            crosshair.visible = false
        }
        
        onClicked: function(mouse) {
            var chartWidth = priceCanvas.width - 60
            var index = Math.floor((mouse.x - 50) / chartWidth * pricePoints.length)
            index = Math.max(0, Math.min(index, pricePoints.length - 1))
            root.pointClicked(index)
        }
    }
    
    // 定时刷新
    Timer {
        interval: updateInterval
        running: model !== null
        repeat: true
        onTriggered: {
            priceCanvas.requestPaint()
            volumeCanvas.requestPaint()
        }
    }
    
    // 数据更新函数
    function updateData() {
        if (!model || model.count === 0) return
        
        pricePoints = []
        volumePoints = []
        
        minPrice = Infinity
        maxPrice = -Infinity
        
        for (var i = 0; i < model.count; i++) {
            var point = model.get(i)
            if (!point) continue
            
            pricePoints.push({
                time: point.time,
                price: point.price,
                avgPrice: point.avgPrice || 0,
                volume: point.volume
            })
            
            volumePoints.push({
                volume: point.volume,
                price: point.price
            })
            
            minPrice = Math.min(minPrice, point.price)
            maxPrice = Math.max(maxPrice, point.price)
            
            if (point.avgPrice > 0) {
                minPrice = Math.min(minPrice, point.avgPrice)
                maxPrice = Math.max(maxPrice, point.avgPrice)
            }
        }
        
        // 以昨收价为基准计算范围
        if (basePrice > 0) {
            var maxChange = Math.max(maxPrice - basePrice, basePrice - minPrice)
            maxChange = Math.max(maxChange, basePrice * 0.05) // 至少5%
            minPrice = basePrice - maxChange
            maxPrice = basePrice + maxChange
        }
        
        priceRange = maxPrice - minPrice
        
        priceCanvas.requestPaint()
        volumeCanvas.requestPaint()
    }
    
    // 格式化成交量
    function formatVolume(vol) {
        if (vol >= 100000000) return (vol / 100000000).toFixed(2) + "亿"
        if (vol >= 10000) return (vol / 10000).toFixed(2) + "万"
        return vol.toString()
    }
    
    // 监听数据变化
    onModelChanged: updateData()
    onBasePriceChanged: updateData()
    
    // 平滑的价格变化动画
    Behavior on minPrice {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }
    Behavior on maxPrice {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }
}
