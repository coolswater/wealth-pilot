/**
 * OrderDialog.qml
 * 订单对话框 - MVVM 架构
 *
 * 使用 OrderViewModel 进行数据绑定
 */

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import WealthPilot.ViewModels 1.0
import WealthPilot.Styles 1.0

/**
 * 订单对话框组件
 */
Dialog {
    id: root
    title: "下单"
    modal: true
    width: 400
    height: 500

    // ViewModel 实例
    OrderViewModel {
        id: viewModel

        // 监听错误
        onErrorOccurred: function (message) {
            errorLabel.text = message
            errorLabel.visible = true
        }

        // 监听订单提交
        onOrderSubmitted: function (orderId) {
            statusLabel.text = "订单已提交: " + orderId
            statusLabel.color = StyleColors.success
            root.close()
        }

        // 监听风控警告
        onRiskWarning: function (message) {
            warningLabel.text = message
            warningLabel.visible = true
        }
    }

    // 背景
    background: Rectangle {
        color: StyleColors.bgElevated
        border.color: StyleColors.border
        border.width: 1
        radius: 12
    }

    // 内容
    contentItem: ColumnLayout {
        spacing: 16

        // ========== 合约信息 ==========
        Frame {
            Layout.fillWidth: true

            background: Rectangle {
                color: StyleColors.bgSurface
                border.color: StyleColors.border
                radius: 8
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 16

                // 合约名称
                Label {
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
                    text: viewModel.lastPrice.toFixed(2)
                    font.pixelSize: 18
                    font.weight: Font.Bold
                    color: viewModel.priceChange >= 0 ? StyleColors.danger : StyleColors.success
                }

                // 涨跌幅
                Label {
                    text: viewModel.priceChangePercent >= 0 ?
                        "+" + viewModel.priceChangePercent.toFixed(2) + "%" :
                        viewModel.priceChangePercent.toFixed(2) + "%"
                    font.pixelSize: 12
                    color: viewModel.priceChange >= 0 ? StyleColors.danger : StyleColors.success
                }
            }
        }

        // ========== 下单参数 ==========
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 12
            columnSpacing: 16

            // 订单类型
            Label {
                text: "订单类型"; color: StyleColors.textSecondary
            }
            ComboBox {
                id: orderTypeCombo
                model: ["限价", "市价", "对手价", "止损"]
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

            // 交易方向
            Label {
                text: "交易方向"; color: StyleColors.textSecondary
            }
            ComboBox {
                id: directionCombo
                model: ["买入", "卖出"]
                currentIndex: viewModel.direction

                onCurrentIndexChanged: {
                    viewModel.setDirection(currentIndex)
                }

                background: Rectangle {
                    color: StyleColors.bgSurface
                    border.color: StyleColors.border
                    radius: 4
                }
            }

            // 开平标志
            Label {
                text: "开平标志"; color: StyleColors.textSecondary
            }
            ComboBox {
                id: openCloseCombo
                model: ["开仓", "平仓"]
                currentIndex: viewModel.openClose

                onCurrentIndexChanged: {
                    viewModel.setOpenClose(currentIndex)
                }

                background: Rectangle {
                    color: StyleColors.bgSurface
                    border.color: StyleColors.border
                    radius: 4
                }
            }

            // 价格
            Label {
                text: "价格"; color: StyleColors.textSecondary
            }
            TextField {
                id: priceField
                text: viewModel.orderPrice.toFixed(2)
                validator: DoubleValidator {
                    bottom: 0; decimals: 2
                }

                onTextChanged: {
                    if (acceptableInput) {
                        viewModel.setOrderPrice(parseFloat(text))
                    }
                }

                background: Rectangle {
                    color: StyleColors.bgSurface
                    border.color: focus ? StyleColors.primary : StyleColors.border
                    radius: 4
                }

                color: StyleColors.textPrimary
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
        }

        // ========== 计算结果 ==========
        Frame {
            Layout.fillWidth: true

            background: Rectangle {
                color: StyleColors.bgSurface
                border.color: StyleColors.border
                radius: 8
            }

            GridLayout {
                anchors.fill: parent
                anchors.margins: 12
                columns: 2
                rowSpacing: 8
                columnSpacing: 16

                Label {
                    text: "需要保证金"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.requiredMargin.toFixed(2)
                    font.weight: Font.Bold
                    color: StyleColors.textPrimary
                }

                Label {
                    text: "预估手续费"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.estimatedCommission.toFixed(2)
                    color: StyleColors.textSecondary
                }

                Label {
                    text: "总资金需求"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.totalRequirement.toFixed(2)
                    font.weight: Font.Bold
                    color: viewModel.fundSufficient ? StyleColors.textPrimary : StyleColors.danger
                }

                Label {
                    text: "可用资金"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.availableFund.toFixed(2)
                    font.weight: Font.Bold
                    color: StyleColors.success
                }
            }
        }

        // ========== 持仓信息 ==========
        Frame {
            Layout.fillWidth: true

            background: Rectangle {
                color: StyleColors.bgSurface
                border.color: StyleColors.border
                radius: 8
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 20

                Label {
                    text: "多头持仓"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.longPosition
                    color: StyleColors.danger
                    font.weight: Font.Bold
                }

                Label {
                    text: "空头持仓"; color: StyleColors.textSecondary
                }
                Label {
                    text: viewModel.shortPosition
                    color: StyleColors.success
                    font.weight: Font.Bold
                }
            }
        }

        // ========== 止损止盈 ==========
        GroupBox {
            Layout.fillWidth: true
            title: "止损止盈"

            background: Rectangle {
                color: StyleColors.bgSurface
                border.color: StyleColors.border
                radius: 8
            }

            label: Label {
                text: "止损止盈"
                color: StyleColors.textSecondary
                font.weight: Font.Bold
            }

            GridLayout {
                anchors.fill: parent
                columns: 3
                rowSpacing: 8
                columnSpacing: 8

                // 止盈
                CheckBox {
                    id: enableTakeProfit
                    checked: viewModel.enableTakeProfit
                    onCheckedChanged: viewModel.setEnableTakeProfit(checked)
                    text: "止盈"

                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        radius: 4
                        border.color: StyleColors.border
                        color: enableTakeProfit.checked ? StyleColors.primary : StyleColors.bgSurface
                    }

                    contentItem: Text {
                        text: enableTakeProfit.text
                        color: StyleColors.textSecondary
                        leftPadding: enableTakeProfit.indicator.width + 8
                    }
                }

                TextField {
                    id: takeProfitField
                    text: viewModel.takeProfitPrice.toFixed(2)
                    enabled: viewModel.enableTakeProfit
                    validator: DoubleValidator {
                        bottom: 0; decimals: 2
                    }
                    onTextChanged: {
                        if (acceptableInput && enabled) {
                            viewModel.setTakeProfitPrice(parseFloat(text))
                        }
                    }

                    background: Rectangle {
                        color: enabled ? StyleColors.bgSurface : StyleColors.bgElevated
                        border.color: StyleColors.border
                        radius: 4
                    }

                    color: StyleColors.textPrimary
                    Layout.fillWidth: true
                }

                Label {
                    text: "盈利: " + viewModel.profitAmount.toFixed(2) + " (" + viewModel.profitRatio.toFixed(1) + "%)"
                    color: StyleColors.success
                    visible: viewModel.enableTakeProfit
                }

                // 止损
                CheckBox {
                    id: enableStopLoss
                    checked: viewModel.enableStopLoss
                    onCheckedChanged: viewModel.setEnableStopLoss(checked)
                    text: "止损"

                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        radius: 4
                        border.color: StyleColors.border
                        color: enableStopLoss.checked ? StyleColors.danger : StyleColors.bgSurface
                    }

                    contentItem: Text {
                        text: enableStopLoss.text
                        color: StyleColors.textSecondary
                        leftPadding: enableStopLoss.indicator.width + 8
                    }
                }

                TextField {
                    id: stopLossField
                    text: viewModel.stopLossPrice.toFixed(2)
                    enabled: viewModel.enableStopLoss
                    validator: DoubleValidator {
                        bottom: 0; decimals: 2
                    }
                    onTextChanged: {
                        if (acceptableInput && enabled) {
                            viewModel.setStopLossPrice(parseFloat(text))
                        }
                    }

                    background: Rectangle {
                        color: enabled ? StyleColors.bgSurface : StyleColors.bgElevated
                        border.color: StyleColors.border
                        radius: 4
                    }

                    color: StyleColors.textPrimary
                    Layout.fillWidth: true
                }

                Label {
                    text: "风险: " + viewModel.riskAmount.toFixed(2) + " (" + viewModel.riskRatio.toFixed(1) + "%)"
                    color: StyleColors.danger
                    visible: viewModel.enableStopLoss
                }
            }
        }

        // ========== 错误/警告提示 ==========
        Label {
            id: errorLabel
            Layout.fillWidth: true
            visible: false
            color: StyleColors.danger
            wrapMode: Text.WordWrap
        }

        Label {
            id: warningLabel
            Layout.fillWidth: true
            visible: false
            color: StyleColors.warning
            wrapMode: Text.WordWrap
        }

        // ========== 状态栏 ==========
        Label {
            id: statusLabel
            Layout.fillWidth: true
            text: viewModel.statusMessage || "就绪"
            color: StyleColors.textSecondary
        }

        // ========== 按钮 ==========
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // 计算按钮
            Button {
                text: "计算"
                enabled: viewModel.calculateCommand.canExecute

                background: Rectangle {
                    color: parent.enabled ?
                        (parent.pressed ? StyleColors.primaryDark : StyleColors.primary) :
                        StyleColors.border
                    radius: 6
                }

                contentItem: Text {
                    text: parent.text
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: viewModel.calculateCommand.execute()
            }

            // 重置按钮
            Button {
                text: "重置"

                background: Rectangle {
                    color: parent.pressed ? StyleColors.bgHover : StyleColors.bgSurface
                    border.color: StyleColors.border
                    radius: 6
                }

                contentItem: Text {
                    text: parent.text
                    color: StyleColors.textPrimary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: viewModel.resetCommand.execute()
            }

            Item {
                Layout.fillWidth: true
            }

            // 取消按钮
            Button {
                text: "取消"

                background: Rectangle {
                    color: parent.pressed ? StyleColors.bgHover : StyleColors.bgSurface
                    border.color: StyleColors.border
                    radius: 6
                }

                contentItem: Text {
                    text: parent.text
                    color: StyleColors.textSecondary
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: root.close()
            }

            // 提交按钮
            Button {
                id: submitBtn
                text: "提交订单"
                enabled: viewModel.submitCommand.canExecute

                background: Rectangle {
                    color: parent.enabled ?
                        (parent.pressed ? StyleColors.primaryDark : StyleColors.primary) :
                        StyleColors.border
                    radius: 6
                }

                contentItem: Text {
                    text: parent.text
                    font.weight: Font.Bold
                    color: "white"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: viewModel.submitCommand.execute()
            }
        }
    }

    // ========== 外部接口 ==========

    /**
     * 设置合约
     */
    function setInstrument(instrumentId, instrumentName, lastPrice, tickSize, volumeMultiple, marginRatio) {
        viewModel.setInstrument(instrumentId, instrumentName, lastPrice, tickSize, volumeMultiple, marginRatio)
    }

    /**
     * 设置账户
     */
    function setAccount(available, margin, frozen) {
        viewModel.setAccount(available, margin, frozen)
    }

    /**
     * 设置持仓
     */
    function setPosition(longPos, shortPos) {
        viewModel.setPosition(longPos, shortPos)
    }
}