/**
 * TradingPanel.qml
 * 交易面板 - MVVM 架构示例
 *
 * 使用 TradingViewModel 进行数据绑定
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import WealthPilot.ViewModels 1.0
import WealthPilot.Styles 1.0

/**
 * 交易面板组件
 */
Item {
    id: root
    implicitWidth: 400
    implicitHeight: 600

    // ViewModel 实例
    TradingViewModel {
        id: viewModel

        // 监听错误
        onErrorOccurred: function (message) {
            errorDialog.show(message)
        }

        // 监听订单提交
        onOrderSubmitted: function (orderId, operation) {
            statusText.text = operation + " 成功: " + orderId
            statusText.color = StyleColors.success
        }

        // 监听风控警告
        onRiskWarning: function (message) {
            warningDialog.show(message)
        }
    }

    // 主布局
    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        // ========== 合约信息区域 ==========
        Frame {
            Layout.fillWidth: true
            Layout.margins: 16

            background: Rectangle {
                color: StyleColors.bgElevated
                border.color: StyleColors.border
                border.width: 1
                radius: 8
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 20

                // 合约名称
                Label {
                    id: instrumentLabel
                    text: viewModel.instrumentName || "选择合约"
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: StyleColors.textPrimary
                }

                Item {
                    Layout.fillWidth: true
                }

                // 最新价
                Label {
                    id: priceLabel
                    text: viewModel.currentPrice.toFixed(2)
                    font.pixelSize: 24
                    font.weight: Font.Bold
                    color: viewModel.priceChange >= 0 ? StyleColors.danger : StyleColors.success

                    // 价格变化动画
                    Behavior on text {
                        enabled: viewModel.currentPrice > 0
                        ColorAnimation {
                            duration: 200
                        }
                    }
                }

                // 涨跌幅
                Label {
                    id: changeLabel
                    text: viewModel.priceChangePercent >= 0 ?
                        "+" + viewModel.priceChangePercent.toFixed(2) + "%" :
                        viewModel.priceChangePercent.toFixed(2) + "%"
                    font.pixelSize: 14
                    font.weight: Font.Medium
                    color: viewModel.priceChange >= 0 ? StyleColors.danger : StyleColors.success
                }
            }
        }

        // ========== 持仓信息区域 ==========
        Frame {
            Layout.fillWidth: true
            Layout.margins: 16

            background: Rectangle {
                color: StyleColors.bgElevated
                border.color: StyleColors.border
                border.width: 1
                radius: 8
            }

            GridLayout {
                anchors.fill: parent
                anchors.margins: 12
                columns: 4
                rowSpacing: 8
                columnSpacing: 16

                // 多头持仓
                Label {
                    text: "多头持仓"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.longPosition
                    font.weight: Font.Bold
                    color: StyleColors.danger
                }

                // 空头持仓
                Label {
                    text: "空头持仓"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.shortPosition
                    font.weight: Font.Bold
                    color: StyleColors.success
                }

                // 多头盈亏
                Label {
                    text: "多头盈亏"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.longProfit.toFixed(2)
                    font.weight: Font.Bold
                    color: viewModel.longProfit >= 0 ? StyleColors.danger : StyleColors.success
                }

                // 空头盈亏
                Label {
                    text: "空头盈亏"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.shortProfit.toFixed(2)
                    font.weight: Font.Bold
                    color: viewModel.shortProfit >= 0 ? StyleColors.success : StyleColors.danger
                }

                // 总盈亏
                Label {
                    text: "总盈亏"; color: StyleColors.textSecondary; font.weight: Font.Bold
                }
                Label {
                    text: viewModel.totalProfit.toFixed(2)
                    font.pixelSize: 16
                    font.weight: Font.Bold
                    color: viewModel.totalProfit >= 0 ? StyleColors.danger : StyleColors.success
                }

                // 可用资金
                Label {
                    text: "可用资金"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.availableFund.toFixed(2)
                    font.weight: Font.Bold
                    color: StyleColors.textPrimary
                }
            }
        }

        // ========== 下单参数区域 ==========
        Frame {
            Layout.fillWidth: true
            Layout.margins: 16

            background: Rectangle {
                color: StyleColors.bgElevated
                border.color: StyleColors.border
                border.width: 1
                radius: 8
            }

            GridLayout {
                anchors.fill: parent
                anchors.margins: 12
                columns: 2
                rowSpacing: 8
                columnSpacing: 16

                // 价格
                Label {
                    text: "价格"; color: StyleColors.textSecondary
                }
                SpinBox {
                    id: priceSpinBox
                    value: viewModel.orderPrice * 100  // 转换为整数（假设最小变动0.01）
                    0
                    to: 1000000
                    stepSize: viewModel.tickSize * 100
                    editable: true

                    onValueChanged: {
                        viewModel.setOrderPrice(value / 100)
                    }

                    background: Rectangle {
                        color: StyleColors.bgSurface
                        border.color: StyleColors.border
                        radius: 4
                    }
                }

                // 数量
                Label {
                    text: "数量"; color: StyleColors.textSecondary
                }
                SpinBox {
                    id: volumeSpinBox
                    value: viewModel.orderVolume
                    1
                    to: 1000
                    stepSize: 1
                    editable: true

                    onValueChanged: {
                        viewModel.setOrderVolume(value)
                    }

                    background: Rectangle {
                        color: StyleColors.bgSurface
                        border.color: StyleColors.border
                        radius: 4
                    }
                }

                // 订单类型
                Label {
                    text: "类型"; color: StyleColors.textSecondary
                }
                ComboBox {
                    id: orderTypeCombo
                    model: ["限价", "市价", "对手价"]
                    currentIndex: viewModel.orderType

                    onCurrentIndexChanged: {
                        viewModel.setOrderType(currentIndex)
                    }

                    background: Rectangle {
                        color: StyleColors.bgSurface
                        border.color: StyleColors.border
                        radius: 4
                    }
                }
            }
        }

        // ========== 交易按钮区域 ==========
        GridLayout {
            Layout.fillWidth: true
            Layout.margins: 16
            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            // 买入开仓
            Button {
                id: buyOpenBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 50

                text: "买入开仓"
                enabled: viewModel.buyOpenCommand.canExecute

                background: Rectangle {
                    color: buyOpenBtn.enabled ?
                        (buyOpenBtn.pressed ? StyleColors.dangerDark : StyleColors.danger) :
                        StyleColors.border
                    radius: 6
                }

                contentItem: Text {
                    text: buyOpenBtn.text
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    viewModel.buyOpenCommand.execute()
                }
            }

            // 卖出开仓
            Button {
                id: sellOpenBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 50

                text: "卖出开仓"
                enabled: viewModel.sellOpenCommand.canExecute

                background: Rectangle {
                    color: sellOpenBtn.enabled ?
                        (sellOpenBtn.pressed ? StyleColors.successDark : StyleColors.success) :
                        StyleColors.border
                    radius: 6
                }

                contentItem: Text {
                    text: sellOpenBtn.text
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    viewModel.sellOpenCommand.execute()
                }
            }

            // 买入平仓
            Button {
                id: buyCloseBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 50

                text: "买入平仓"
                enabled: viewModel.buyCloseCommand.canExecute

                background: Rectangle {
                    color: buyCloseBtn.enabled ? StyleColors.bgHover : StyleColors.bgSurface
                    border.color: StyleColors.border
                    radius: 6
                }

                contentItem: Text {
                    text: buyCloseBtn.text
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: buyCloseBtn.enabled ? StyleColors.textPrimary : StyleColors.textDisabled
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    viewModel.buyCloseCommand.execute()
                }
            }

            // 卖出平仓
            Button {
                id: sellCloseBtn
                Layout.fillWidth: true
                Layout.preferredHeight: 50

                text: "卖出平仓"
                enabled: viewModel.sellCloseCommand.canExecute

                background: Rectangle {
                    color: sellCloseBtn.enabled ? StyleColors.bgHover : StyleColors.bgSurface
                    border.color: StyleColors.border
                    radius: 6
                }

                contentItem: Text {
                    text: sellCloseBtn.text
                    font.pixelSize: 14
                    font.weight: Font.Bold
                    color: sellCloseBtn.enabled ? StyleColors.textPrimary : StyleColors.textDisabled
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    viewModel.sellCloseCommand.execute()
                }
            }
        }

        // ========== 状态栏 ==========
        Frame {
            Layout.fillWidth: true
            Layout.margins: 16

            background: Rectangle {
                color: StyleColors.bgElevated
                border.color: StyleColors.border
                border.width: 1
                radius: 4
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 12

                // 加载指示器
                BusyIndicator {
                    running: viewModel.isLoading
                    visible: viewModel.isLoading
                    Layout.preferredWidth: 24
                    Layout.preferredHeight: 24
                }

                // 状态文本
                Label {
                    id: statusText
                    text: viewModel.statusMessage || "就绪"
                    color: StyleColors.textSecondary
                    Layout.fillWidth: true
                }

                // 刷新按钮
                Button {
                    text: "刷新"
                    implicitWidth: 60
                    implicitHeight: 24

                    background: Rectangle {
                        color: parent.pressed ? StyleColors.primaryDark :
                            parent.hovered ? StyleColors.primaryHover : StyleColors.primary
                        radius: 4
                    }

                    contentItem: Text {
                        text: parent.text
                        font.pixelSize: 12
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: {
                        viewModel.refreshCommand.execute()
                    }
                }
            }
        }

        // 弹性空间
        Item {
            Layout.fillHeight: true
        }
    }

    // ========== 错误对话框 ==========
    Dialog {
        id: errorDialog
        title: "错误"
        modal: true
        anchors.centerIn: parent

        property alias message: errorLabel.text

        function show(msg) {
            message = msg
            open()
        }

        Label {
            id: errorLabel
            color: StyleColors.danger
        }

        standardButtons: Dialog.Ok

        background: Rectangle {
            color: StyleColors.bgElevated
            border.color: StyleColors.danger
            radius: 8
        }
    }

    // ========== 警告对话框 ==========
    Dialog {
        id: warningDialog
        title: "风控警告"
        modal: true
        anchors.centerIn: parent

        property alias message: warningLabel.text

        function show(msg) {
            message = msg
            open()
        }

        Label {
            id: warningLabel
            color: StyleColors.warning
        }

        standardButtons: Dialog.Ok

        background: Rectangle {
            color: StyleColors.bgElevated
            border.color: StyleColors.warning
            radius: 8
        }
    }

    // ========== 外部接口 ==========

    /**
     * 设置合约
     */
    function setInstrument(instrumentId, instrumentName, exchange) {
        viewModel.setInstrument(instrumentId, instrumentName, exchange)
    }

    /**
     * 设置价格
     */
    function setPrice(price) {
        viewModel.setOrderPrice(price)
    }

    /**
     * 设置数量
     */
    function setVolume(volume) {
        viewModel.setOrderVolume(volume)
    }
}