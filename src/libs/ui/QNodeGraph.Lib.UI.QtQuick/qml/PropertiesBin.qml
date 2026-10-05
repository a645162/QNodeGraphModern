import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

Rectangle {
    id: propertiesBin

    required property var controller
    property bool collapsed: false
    property int colorRow: -1
    property string colorProperty: ""

    color: Theme.panelBg
    border.color: Theme.panelBorder
    radius: Theme.radius

    ColorDialog {
        id: colorDialog
        title: qsTr("选择背景颜色")
        onAccepted: {
            if (propertiesBin.colorRow >= 0 &&
                    propertiesBin.colorProperty.length > 0) {
                propertiesBin.controller.setNodeProperty(
                    propertiesBin.colorRow, propertiesBin.colorProperty,
                    selectedColor.toString())
            }
        }
    }

    ToolButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 6
        anchors.rightMargin: 6
        text: propertiesBin.collapsed ? qsTr("+") : qsTr("-")
        onClicked: propertiesBin.collapsed = !propertiesBin.collapsed
        ToolTip.visible: hovered
        ToolTip.text: propertiesBin.collapsed ? qsTr("展开属性")
                                               : qsTr("折叠属性")
        z: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 38
        anchors.bottomMargin: 12
        spacing: 10

        Label {
            visible: !propertiesBin.collapsed
            text: qsTr("属性")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: 16
        }

        Label {
            visible: !propertiesBin.collapsed
            Layout.fillWidth: true
            text: propertiesBin.controller.selectedRow >= 0
                  ? qsTr("节点 %1").arg(propertiesBin.controller.selectedRow + 1)
                  : qsTr("请选择节点")
            color: Theme.textSecondary
            elide: Text.ElideRight
        }

        Rectangle {
            visible: !propertiesBin.collapsed
            Layout.fillWidth: true
            height: 1
            color: Theme.panelBorder
        }

        ListView {
            id: propertyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: propertiesBin.controller.selectedProperties
            visible: !propertiesBin.collapsed

            delegate: ColumnLayout {
                required property var modelData
                width: propertyList.width
                spacing: 4

                Label {
                    Layout.fillWidth: true
                    text: modelData.name
                    color: Theme.nodeBodyText
                    elide: Text.ElideRight
                }

                Label {
                    visible: String(modelData.description).length > 0
                    Layout.fillWidth: true
                    text: String(modelData.description)
                    color: Theme.nodeMutedText
                    wrapMode: Text.WordWrap
                    font.pixelSize: 11
                }

                RowLayout {
                    Layout.fillWidth: true

                    CheckBox {
                        visible: modelData.editor === "checkbox" ||
                                 modelData.valueType === "bool"
                        checked: Boolean(modelData.value)
                        onToggled: propertiesBin.controller.setNodeProperty(
                            propertiesBin.controller.selectedRow,
                            modelData.name, checked)
                    }

                    Slider {
                        visible: modelData.editor === "slider"
                        Layout.fillWidth: true
                        from: Number(modelData.minimum)
                        to: Number(modelData.maximum)
                        stepSize: Number(modelData.step) > 0
                                  ? Number(modelData.step) : 1
                        value: Number(modelData.value)
                        onValueChanged: {
                            if (pressed)
                                propertiesBin.controller.setNodeProperty(
                                            propertiesBin.controller.selectedRow,
                                            modelData.name, value)
                        }
                    }

                    TextField {
                        visible: modelData.editor === "slider"
                        Layout.preferredWidth: 64
                        text: Number(modelData.value).toLocaleString(
                                  Qt.locale(), 'f',
                                  Number(modelData.step) < 1 ? 2 : 0)
                        selectByMouse: true
                        inputMethodHints: Qt.ImhFormattedNumbersOnly
                        onEditingFinished: propertiesBin.controller.setNodeProperty(
                            propertiesBin.controller.selectedRow,
                            modelData.name, text)
                    }

                    ComboBox {
                        visible: modelData.editor === "combobox"
                        Layout.fillWidth: true
                        model: modelData.choices
                        currentIndex: Math.max(0, indexOfValue(
                            String(modelData.value)))
                        onActivated: propertiesBin.controller.setNodeProperty(
                                         propertiesBin.controller.selectedRow,
                                         modelData.name, currentText)
                    }

                    TextField {
                        visible: modelData.editor !== "checkbox" &&
                                 modelData.editor !== "slider" &&
                                 modelData.editor !== "combobox" &&
                                 modelData.editor !== "matrix" &&
                                 modelData.editor !== "color" &&
                                 modelData.valueType !== "bool"
                        Layout.fillWidth: true
                        text: String(modelData.value)
                        selectByMouse: true
                        onEditingFinished: propertiesBin.controller.setNodeProperty(
                            propertiesBin.controller.selectedRow,
                            modelData.name, text)
                    }

                    Button {
                        visible: modelData.editor === "color" ||
                                 modelData.name === "background"
                        text: qsTr("取色")
                        onClicked: {
                            propertiesBin.colorRow = propertiesBin.controller.selectedRow
                            propertiesBin.colorProperty = modelData.name
                            colorDialog.selectedColor = String(modelData.value)
                            colorDialog.open()
                        }
                    }

                    TextField {
                        visible: modelData.editor === "matrix"
                        Layout.fillWidth: true
                        text: String(modelData.value)
                        selectByMouse: true
                        placeholderText: modelData.matrixRows > 0 &&
                                         modelData.matrixCols > 0
                                         ? qsTr("%1 x %2 矩阵").arg(
                                               modelData.matrixRows,
                                               modelData.matrixCols)
                                         : qsTr("矩阵")
                        onEditingFinished: propertiesBin.controller.setNodeProperty(
                            propertiesBin.controller.selectedRow,
                            modelData.name, text)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: !propertiesBin.collapsed && propertyList.count === 0
                        text: qsTr("没有可编辑属性")
                color: Theme.nodeMutedText
            }
        }
    }
}
