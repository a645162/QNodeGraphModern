import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QNodeGraph.UI 1.0

Rectangle {
    id: nodesPalette

    required property var controller
    property var helpProvider: null
    property Item dropTarget: null
    property bool collapsed: false
    color: Theme.panelBg
    border.color: Theme.panelBorder
    radius: Theme.radius

    NodePaletteModel {
        id: paletteModel
        controller: nodesPalette.controller
        hideFixedNodes: true
    }

    ToolButton {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: 6
        anchors.leftMargin: 6
        text: nodesPalette.collapsed ? qsTr("+") : qsTr("-")
        onClicked: nodesPalette.collapsed = !nodesPalette.collapsed
        ToolTip.visible: hovered
        ToolTip.text: nodesPalette.collapsed ? qsTr("展开组件库")
                                               : qsTr("折叠组件库")
        z: 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        anchors.topMargin: 38
        anchors.bottomMargin: 10
        spacing: 8

        RowLayout {
            visible: !nodesPalette.collapsed
            Layout.fillWidth: true

            ComboBox {
                id: nodeTypeCombo
                Layout.fillWidth: true
                model: paletteModel
                textRole: "nodeName"
                valueRole: "nodeType"
            }

            Button {
                text: qsTr("添加")
                onClicked: {
                    if (nodeTypeCombo.currentValue)
                        nodesPalette.controller.addNodeType(
                                    nodeTypeCombo.currentValue)
                }
            }
        }

        Label {
            visible: !nodesPalette.collapsed
            text: qsTr("组件库")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: 16
        }

        TextField {
            visible: !nodesPalette.collapsed
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("搜索组件")
            selectByMouse: true
            onTextChanged: paletteModel.filter = text
        }

        ListView {
            id: nodeList
            objectName: "workflowNodePaletteList"
            visible: !nodesPalette.collapsed
            property var nodeController: nodesPalette.controller
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: paletteModel

            delegate: Rectangle {
                id: paletteDelegate
                objectName: "workflowNodePaletteDelegate"
                required property string nodeType
                required property string nodeName
                required property string category
                required property int inputPortCount
                required property int outputPortCount
                width: nodeList.width
                height: 52
                color: mouseArea.containsMouse ? Theme.nodeBgSelected
                                               : Theme.nodeBg
                border.color: mouseArea.containsMouse ? Theme.nodeBorderSelected
                                                      : Theme.nodeBorder
                radius: Theme.radiusSmall

                Drag.dragType: Drag.Automatic
                Drag.active: paletteDelegate.dragging
                Drag.supportedActions: Qt.CopyAction
                Drag.keys: ["qnodegraph.node"]
                Drag.mimeData: { "text/plain": nodeType }
                property bool dragging: false
                property bool dropHandled: false

                Column {
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    anchors.right: helpButton.left
                    anchors.rightMargin: 8
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Label {
                        text: nodeName
                        color: Theme.textPrimary
                        font.bold: true
                    }

                    Label {
                        text: qsTr("%1  |  %2 in  %3 out")
                              .arg(category)
                              .arg(inputPortCount)
                              .arg(outputPortCount)
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }

                ToolButton {
                    id: helpButton
                    anchors.right: parent.right
                    anchors.rightMargin: 4
                    anchors.verticalCenter: parent.verticalCenter
                    text: "?"
                    visible: nodesPalette.helpProvider !== null
                    onClicked: nodesPalette.helpProvider.openFor(nodeType)
                    ToolTip.visible: hovered
                    ToolTip.text: qsTr("查看帮助")
                    z: 2
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    property point pressPoint

                    onPressed: function(mouse) {
                        pressPoint = Qt.point(mouse.x, mouse.y)
                        paletteDelegate.dropHandled = false
                    }
                    onPositionChanged: function(mouse) {
                        if (!paletteDelegate.dragging &&
                            (Math.abs(mouse.x - pressPoint.x) > 8 ||
                             Math.abs(mouse.y - pressPoint.y) > 8)) {
                            paletteDelegate.dragging = true
                        }
                    }
                    onReleased: function(mouse) {
                        if (paletteDelegate.dragging) {
                            paletteDelegate.Drag.drop()
                            if (!paletteDelegate.dropHandled && nodesPalette.dropTarget) {
                                var targetPoint = nodesPalette.dropTarget.mapFromItem(
                                            paletteDelegate, mouse.x, mouse.y)
                                nodesPalette.dropTarget.controller.addNodeTypeAt(
                                            nodeType, targetPoint.x, targetPoint.y, false)
                            }
                            paletteDelegate.dragging = false
                        } else {
                            nodeList.nodeController.addNodeType(nodeType)
                        }
                    }
                }
            }
        }
    }
}
