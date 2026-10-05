import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QNodeGraph.UI 1.0

Rectangle {
    id: propertiesBin

    required property var controller
    property var helpProvider: null
    property bool collapsed: false
    property int colorRow: -1
    property string colorProperty: ""
    property bool advancedExpanded: false
    property bool developerExpanded: false

    color: Theme.panelBg
    border.color: Theme.panelBorder
    radius: Theme.radius

    function propertiesFor(visibility) {
        var values = propertiesBin.controller.selectedProperties
        return values.filter(function (value) {
            return String(value.visibility || "basic") === visibility
        })
    }

    function valueLabel(modelData) {
        var unit = String(modelData.unit || "")
        return unit.length > 0
                ? qsTr("%1 (%2)").arg(modelData.name).arg(unit)
                : String(modelData.name)
    }

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

    Component {
        id: propertyEditor

        ColumnLayout {
            required property var modelData
            Layout.fillWidth: true
            spacing: 4

            Label {
                Layout.fillWidth: true
                text: propertiesBin.valueLabel(modelData)
                color: Theme.nodeBodyText
                elide: Text.ElideRight
                HoverHandler { id: propertyHover }
                ToolTip.visible: propertyHover.hovered &&
                                 String(modelData.description || "").length > 0
                ToolTip.text: String(modelData.description || "")
            }

            Label {
                visible: String(modelData.description || "").length > 0
                Layout.fillWidth: true
                text: String(modelData.description || "")
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
                    to: Math.max(Number(modelData.minimum) + 1,
                                 Number(modelData.maximum))
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
                    visible: modelData.editor === "slider" ||
                             modelData.editor === "spinbox"
                    Layout.preferredWidth: 72
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
                             modelData.editor !== "spinbox" &&
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

    ToolButton {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 6
        anchors.rightMargin: 42
        text: "?"
        enabled: propertiesBin.helpProvider !== null &&
                 propertiesBin.controller.selectedRow >= 0
        onClicked: {
            if (enabled) {
                var typeId = propertiesBin.controller.data(
                            propertiesBin.controller.index(
                                propertiesBin.controller.selectedRow, 0),
                            GraphController.NodeTypeRole)
                propertiesBin.helpProvider.openFor(String(typeId))
            }
        }
        ToolTip.visible: hovered
        ToolTip.text: qsTr("查看节点帮助")
        z: 2
    }

    ScrollView {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.topMargin: 38
        anchors.bottomMargin: 12
        clip: true
        visible: !propertiesBin.collapsed

        ColumnLayout {
            width: propertiesBin.width - 24
            spacing: 10

            Label {
                Layout.fillWidth: true
                text: qsTr("属性")
                color: Theme.textPrimary
                font.bold: true
                font.pixelSize: 16
            }

            Label {
                Layout.fillWidth: true
                text: propertiesBin.controller.selectedRow >= 0
                      ? qsTr("节点 %1").arg(
                            propertiesBin.controller.selectedRow + 1)
                      : qsTr("请选择节点")
                color: Theme.textSecondary
                elide: Text.ElideRight
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: Theme.panelBorder
            }

            Label {
                visible: propertiesBin.propertiesFor("basic").length === 0 &&
                         propertiesBin.propertiesFor("advanced").length === 0 &&
                         propertiesBin.propertiesFor("developer").length === 0
                Layout.fillWidth: true
                text: qsTr("没有可编辑属性")
                color: Theme.nodeMutedText
            }

            Repeater {
                model: propertiesBin.propertiesFor("basic")
                delegate: propertyEditor
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: propertiesBin.propertiesFor("advanced").length > 0
                spacing: 6

                ToolButton {
                    Layout.fillWidth: true
                    text: (propertiesBin.advancedExpanded ? "- " : "+ ") +
                          qsTr("高级参数")
                    horizontalAlignment: Text.AlignLeft
                    onClicked: propertiesBin.advancedExpanded =
                                   !propertiesBin.advancedExpanded
                }

                Repeater {
                    model: propertiesBin.advancedExpanded
                           ? propertiesBin.propertiesFor("advanced") : []
                    delegate: propertyEditor
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                visible: propertiesBin.propertiesFor("developer").length > 0
                spacing: 6

                ToolButton {
                    Layout.fillWidth: true
                    text: (propertiesBin.developerExpanded ? "- " : "+ ") +
                          qsTr("开发者参数")
                    horizontalAlignment: Text.AlignLeft
                    onClicked: propertiesBin.developerExpanded =
                                   !propertiesBin.developerExpanded
                }

                Repeater {
                    model: propertiesBin.developerExpanded
                           ? propertiesBin.propertiesFor("developer") : []
                    delegate: propertyEditor
                }
            }
        }
    }
}
