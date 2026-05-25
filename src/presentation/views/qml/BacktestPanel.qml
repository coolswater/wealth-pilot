/**
 * BacktestPanel.qml
 * @brief 回测面板 - MVVM 架构
 *
 * @details 功能：
 * - 策略选择和参数配置
 * - 回测执行控制
 * - 结果展示和图表
 * - 报告导出
 *
 * @author WealthPilot Team
 * @version 1.0.0
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtCharts

import WealthPilot.ViewModels 1.0
import WealthPilot.Styles 1.0

/**
 * @brief 回测面板
 */
Rectangle {
    id: root

    color: StyleColors.bgPrimary

    // ViewModel
    BacktestViewModel {
        id: viewModel

        onBacktestCompleted: function (success, message) {
            if (success) {
                statusText.text = "回测完成"
                statusText.color = StyleColors.success
            } else {
                statusText.text = message
                statusText.color = StyleColors.danger
            }
        }

        onProgressUpdated: function (percent, currentDate) {
            progressBar.value = percent / 100
            progressLabel.text = currentDate + " (" + percent + "%)"
        }
    }

    // 布局
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 20

        // ========== 标题栏 ==========
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "策略回测"
                font.pixelSize: 24
                font.weight: Font.Bold
                color: StyleColors.textPrimary
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                id: statusText
                text: viewModel.statusMessage
                font.pixelSize: 14
                color: StyleColors.textSecondary
            }
        }

        // ========== 主内容区 ==========
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            orientation: Qt.Horizontal

            // ========== 左侧：参数配置 ==========
            Rectangle {
                SplitView.preferredWidth: 320
                SplitView.minimumWidth: 280
                color: StyleColors.bgSecondary
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    Text {
                        text: "回测参数"
                        font.pixelSize: 16
                        font.weight: Font.Medium
                        color: StyleColors.textPrimary
                    }

                    // 策略选择
                    ColumnLayout {
                        spacing: 8

                        Text {
                            text: "策略"
                            font.pixelSize: 12
                            color: StyleColors.textSecondary
                        }

                        ComboBox {
                            id: strategyCombo
                            Layout.fillWidth: true
                            model: ["均线策略", "突破策略", "动量策略", "均值回归"]

                            onCurrentTextChanged: {
                                viewModel.setStrategyName(currentText)
                            }

                            background: Rectangle {
                                color: StyleColors.bgElevated
                                border.color: StyleColors.border
                                radius: 4
                            }
                        }
                    }

                    // 日期范围
                    RowLayout {
                        spacing: 12

                        ColumnLayout {
                            spacing: 8

                            Text {
                                text: "开始日期"
                                font.pixelSize: 12
                                color: StyleColors.textSecondary
                            }

                            TextField {
                                id: startDateField
                                Layout.preferredWidth: 120
                                text: viewModel.startDate
                                placeholderText: "YYYY-MM-DD"

                                onTextChanged: viewModel.setStartDate(text)

                                background: Rectangle {
                                    color: StyleColors.bgElevated
                                    border.color: parent.activeFocus ? StyleColors.primary : StyleColors.border
                                    radius: 4
                                }

                                color: StyleColors.textPrimary
                            }
                        }

                        ColumnLayout {
                            spacing: 8

                            Text {
                                text: "结束日期"
                                font.pixelSize: 12
                                color: StyleColors.textSecondary
                            }

                            TextField {
                                id: endDateField
                                Layout.preferredWidth: 120
                                text: viewModel.endDate
                                placeholderText: "YYYY-MM-DD"

                                onTextChanged: viewModel.setEndDate(text)

                                background: Rectangle {
                                    color: StyleColors.bgElevated
                                    border.color: parent.activeFocus ? StyleColors.primary : StyleColors.border
                                    radius: 4
                                }

                                color: StyleColors.textPrimary
                            }
                        }
                    }

                    // 初始资金
                    ColumnLayout {
                        spacing: 8

                        Text {
                            text: "初始资金"
                            font.pixelSize: 12
                            color: StyleColors.textSecondary
                        }

                        TextField {
                            id: capitalField
                            Layout.fillWidth: true
                            text: viewModel.initialCapital
                            validator: DoubleValidator {
                                bottom: 1000
                            }

                            onTextChanged: {
                                var value = parseFloat(text)
                                if (!isNaN(value)) {
                                    viewModel.setInitialCapital(value)
                                }
                            }

                            background: Rectangle {
                                color: StyleColors.bgElevated
                                border.color: parent.activeFocus ? StyleColors.primary : StyleColors.border
                                radius: 4
                            }

                            color: StyleColors.textPrimary
                        }
                    }

                    // 手续费率
                    ColumnLayout {
                        spacing: 8

                        Text {
                            text: "手续费率 (%)"
                            font.pixelSize: 12
                            color: StyleColors.textSecondary
                        }

                        Slider {
                            id: commissionSlider
                            Layout.fillWidth: true
                            0
                            to: 0.1
                            stepSize: 0.0001
                            value: viewModel.commissionRate

                            onValueChanged: viewModel.setCommissionRate(value)

                            background: Rectangle {
                                x: commissionSlider.leftPadding
                                y: commissionSlider.topPadding + commissionSlider.availableHeight / 2 - height / 2
                                width: commissionSlider.availableWidth
                                height: 4
                                radius: 2
                                color: StyleColors.bgElevated

                                Rectangle {
                                    width: commissionSlider.visualPosition * parent.width
                                    height: parent.height
                                    color: StyleColors.primary
                                    radius: 2
                                }
                            }

                            handle: Rectangle {
                                x: commissionSlider.leftPadding + commissionSlider.visualPosition * (commissionSlider.availableWidth - width)
                                y: commissionSlider.topPadding + commissionSlider.availableHeight / 2 - height / 2
                                width: 16
                                height: 16
                                radius: 8
                                color: commissionSlider.pressed ? StyleColors.primaryHover : StyleColors.primary
                            }
                        }

                        Text {
                            text: (viewModel.commissionRate * 100).toFixed(2) + "%"
                            font.pixelSize: 12
                            color: StyleColors.textSecondary
                        }
                    }

                    // 滑点
                    ColumnLayout {
                        spacing: 8

                        Text {
                            text: "滑点 (点)"
                            font.pixelSize: 12
                            color: StyleColors.textSecondary
                        }

                        SpinBox {
                            id: slippageSpinBox
                            Layout.fillWidth: true
                            0
                            to: 100
                            stepSize: 1
                            value: viewModel.slippage

                            onValueChanged: viewModel.setSlippage(value)

                            background: Rectangle {
                                color: StyleColors.bgElevated
                                border.color: StyleColors.border
                                radius: 4
                            }

                            contentItem: Text {
                                text: slippageSpinBox.value
                                font.pixelSize: 14
                                color: StyleColors.textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }

                            up.indicator: Rectangle {
                                x: parent.width - width
                                height: parent.height
                                width: 40
                                color: up.pressed ? StyleColors.bgHover : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "+"
                                    color: StyleColors.textSecondary
                                }
                            }

                            down.indicator: Rectangle {
                                x: 0
                                height: parent.height
                                width: 40
                                color: down.pressed ? StyleColors.bgHover : "transparent"

                                Text {
                                    anchors.centerIn: parent
                                    text: "-"
                                    color: StyleColors.textSecondary
                                }
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }

                    // 验证错误
                    Rectangle {
                        Layout.fillWidth: true
                        height: validationErrorText.implicitHeight + 16
                        color: "transparent"
                        visible: !viewModel.paramsValid

                        Rectangle {
                            anchors.fill: parent
                            color: StyleColors.danger
                            opacity: 0.1
                            radius: 4
                        }

                        Text {
                            id: validationErrorText
                            anchors.fill: parent
                            anchors.margins: 8
                            text: viewModel.validationError
                            font.pixelSize: 12
                            color: StyleColors.danger
                            wrapMode: Text.WordWrap
                        }
                    }

                    // 控制按钮
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        // 运行按钮
                        Button {
                            id: runButton
                            Layout.fillWidth: true
                            text: "运行回测"
                            enabled: viewModel.runCommand.canExecute

                            onClicked: viewModel.runCommand.execute()

                            background: Rectangle {
                                color: runButton.enabled ?
                                    (runButton.pressed ? StyleColors.primaryDark : StyleColors.primary) :
                                    StyleColors.bgElevated
                                radius: 4
                            }

                            contentItem: Text {
                                text: runButton.text
                                font.pixelSize: 14
                                font.weight: Font.Medium
                                color: runButton.enabled ? StyleColors.textPrimary : StyleColors.textDisabled
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }

                        // 停止按钮
                        Button {
                            id: stopButton
                            Layout.preferredWidth: 80
                            text: "停止"
                            enabled: viewModel.stopCommand.canExecute

                            onClicked: viewModel.stopCommand.execute()

                            background: Rectangle {
                                color: stopButton.enabled ?
                                    (stopButton.pressed ? StyleColors.danger : StyleColors.bgElevated) :
                                    "transparent"
                                border.color: stopButton.enabled ? StyleColors.danger : StyleColors.border
                                radius: 4
                            }

                            contentItem: Text {
                                text: stopButton.text
                                font.pixelSize: 14
                                color: stopButton.enabled ? StyleColors.danger : StyleColors.textDisabled
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                            }
                        }
                    }

                    // 重置按钮
                    Button {
                        id: resetButton
                        Layout.fillWidth: true
                        text: "重置"

                        onClicked: viewModel.resetCommand.execute()

                        background: Rectangle {
                            color: resetButton.pressed ? StyleColors.bgHover : StyleColors.bgElevated
                            border.color: StyleColors.border
                            radius: 4
                        }

                        contentItem: Text {
                            text: resetButton.text
                            font.pixelSize: 14
                            color: StyleColors.textSecondary
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }
                    }
                }
            }

            // ========== 右侧：结果展示 ==========
            Rectangle {
                SplitView.fillWidth: true
                color: StyleColors.bgSecondary
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 16

                    // 标签页
                    TabBar {
                        id: resultTabBar
                        Layout.fillWidth: true

                        background: Rectangle {
                            color: "transparent"
                        }

                        TabButton {
                            text: "概览"

                            background: Rectangle {
                                color: resultTabBar.currentIndex === 0 ? StyleColors.bgElevated : "transparent"
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 14
                                color: resultTabBar.currentIndex === 0 ? StyleColors.textPrimary : StyleColors.textSecondary
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        TabButton {
                            text: "资金曲线"

                            background: Rectangle {
                                color: resultTabBar.currentIndex === 1 ? StyleColors.bgElevated : "transparent"
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 14
                                color: resultTabBar.currentIndex === 1 ? StyleColors.textPrimary : StyleColors.textSecondary
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }

                        TabButton {
                            text: "交易记录"

                            background: Rectangle {
                                color: resultTabBar.currentIndex === 2 ? StyleColors.bgElevated : "transparent"
                                radius: 4
                            }

                            contentItem: Text {
                                text: parent.text
                                font.pixelSize: 14
                                color: resultTabBar.currentIndex === 2 ? StyleColors.textPrimary : StyleColors.textSecondary
                                horizontalAlignment: Text.AlignHCenter
                            }
                        }
                    }

                    // 进度条
                    RowLayout {
                        Layout.fillWidth: true
                        visible: viewModel.state === 1  // Running

                        ProgressBar {
                            id: progressBar
                            Layout.fillWidth: true
                            0
                            to: 1
                            value: 0

                            background: Rectangle {
                                implicitWidth: 200
                                implicitHeight: 4
                                color: StyleColors.bgElevated
                                radius: 2
                            }

                            contentItem: Item {
                                implicitWidth: 200
                                implicitHeight: 4

                                Rectangle {
                                    width: progressBar.visualPosition * parent.width
                                    height: parent.height
                                    radius: 2
                                    color: StyleColors.primary
                                }
                            }
                        }

                        Text {
                            id: progressLabel
                            text: "0%"
                            font.pixelSize: 12
                            color: StyleColors.textSecondary
                        }
                    }

                    // 内容区
                    StackLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        currentIndex: resultTabBar.currentIndex

                        // 概览
                        Item {
                            id: overviewTab

                            GridLayout {
                                anchors.fill: parent
                                columns: 4
                                rowSpacing: 16
                                columnSpacing: 16

                                // 收益指标
                                MetricCard {
                                    title: "总收益率"
                                    value: viewModel.totalReturn.toFixed(2) + "%"
                                    color: viewModel.totalReturn >= 0 ? StyleColors.success : StyleColors.danger
                                }

                                MetricCard {
                                    title: "年化收益"
                                    value: viewModel.annualizedReturn.toFixed(2) + "%"
                                    color: viewModel.annualizedReturn >= 0 ? StyleColors.success : StyleColors.danger
                                }

                                MetricCard {
                                    title: "最大回撤"
                                    value: viewModel.maxDrawdown.toFixed(2) + "%"
                                    color: StyleColors.danger
                                }

                                MetricCard {
                                    title: "夏普比率"
                                    value: viewModel.sharpeRatio.toFixed(2)
                                    color: viewModel.sharpeRatio >= 1 ? StyleColors.success : StyleColors.warning
                                }

                                MetricCard {
                                    title: "胜率"
                                    value: viewModel.winRate.toFixed(1) + "%"
                                    color: StyleColors.info
                                }

                                MetricCard {
                                    title: "盈亏比"
                                    value: viewModel.profitFactor.toFixed(2)
                                    color: viewModel.profitFactor >= 1 ? StyleColors.success : StyleColors.danger
                                }

                                MetricCard {
                                    title: "总交易次数"
                                    value: viewModel.totalTrades.toString()
                                    color: StyleColors.textPrimary
                                }

                                MetricCard {
                                    title: "最终资金"
                                    value: viewModel.finalCapital.toFixed(2)
                                    color: StyleColors.textPrimary
                                }
                            }
                        }

                        // 资金曲线
                        Item {
                            id: equityCurveTab

                            ChartView {
                                anchors.fill: parent
                                anchors.margins: -10
                                backgroundColor: StyleColors.bgSecondary
                                legend.visible: false

                                ValueAxis {
                                    id: valueAxisX
                                    visible: false
                                }

                                ValueAxis {
                                    id: valueAxisY
                                    labelsColor: StyleColors.textSecondary
                                    gridLineColor: StyleColors.border
                                    labelsFont.pixelSize: 10
                                }

                                LineSeries {
                                    id: equitySeries
                                    axisX: valueAxisX
                                    axisY: valueAxisY
                                    color: StyleColors.primary
                                    width: 2
                                }
                            }
                        }

                        // 交易记录
                        Item {
                            id: tradesTab

                            ListView {
                                anchors.fill: parent
                                clip: true

                                model: viewModel.getTradeHistory()

                                delegate: Rectangle {
                                    width: parent.width
                                    height: 40
                                    color: index % 2 === 0 ? "transparent" : StyleColors.bgElevated

                                    RowLayout {
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12

                                        Text {
                                            text: modelData.date
                                            font.pixelSize: 12
                                            color: StyleColors.textSecondary
                                            Layout.preferredWidth: 100
                                        }

                                        Text {
                                            text: modelData.action
                                            font.pixelSize: 12
                                            color: modelData.action === "买入" ? StyleColors.success : StyleColors.danger
                                            Layout.preferredWidth: 60
                                        }

                                        Text {
                                            text: modelData.symbol
                                            font.pixelSize: 12
                                            color: StyleColors.textPrimary
                                            Layout.preferredWidth: 80
                                        }

                                        Text {
                                            text: modelData.price.toFixed(2)
                                            font.pixelSize: 12
                                            color: StyleColors.textPrimary
                                            Layout.preferredWidth: 80
                                        }

                                        Text {
                                            text: modelData.quantity.toString()
                                            font.pixelSize: 12
                                            color: StyleColors.textPrimary
                                            Layout.preferredWidth: 60
                                        }

                                        Text {
                                            text: modelData.profit.toFixed(2)
                                            font.pixelSize: 12
                                            color: modelData.profit >= 0 ? StyleColors.success : StyleColors.danger
                                            Layout.fillWidth: true
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // 导出按钮
                    Button {
                        id: exportButton
                        Layout.alignment: Qt.AlignRight
                        text: "导出报告"
                        enabled: viewModel.exportCommand.canExecute

                        onClicked: viewModel.exportCommand.execute()

                        background: Rectangle {
                            color: exportButton.enabled ?
                                (exportButton.pressed ? StyleColors.primaryDark : StyleColors.primary) :
                                StyleColors.bgElevated
                            radius: 4
                        }

                        contentItem: Row {
                            spacing: 8

                            Text {
                                text: "📄"
                                font.pixelSize: 14
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                text: exportButton.text
                                font.pixelSize: 14
                                color: exportButton.enabled ? StyleColors.textPrimary : StyleColors.textDisabled
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }
        }
    }

    // ========== 组件 ==========

    /**
     * @brief 指标卡片组件
     */
    component MetricCard: Rectangle {
        property string title: ""
        property string value: ""
        property color valueColor: StyleColors.textPrimary

        color: StyleColors.bgElevated
        radius: 8

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: title
                font.pixelSize: 12
                color: StyleColors.textSecondary
            }

            Text {
                text: value
                font.pixelSize: 20
                font.weight: Font.Bold
                color: valueColor
            }
        }
    }
}