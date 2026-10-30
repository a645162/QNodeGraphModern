import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: tree

    required property var controller
    property Item dropTarget: null
    color: Theme.panelBg
    border.color: Theme.panelBorder
    radius: Theme.radius

    // 分类名来自 typeId 前缀（op/viewer），属于内部标识；面板上显示可读的
    // 中文/英文名称，未知前缀原样回退，避免把标识硬翻译坏。
    function categoryLabel(value) {
        if (value === "op")
            return qsTr("算子")
        if (value === "viewer")
            return qsTr("查看器")
        return value
    }

    NodePaletteModel {
        id: treeModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: qsTr("节点树")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: 16
        }

        ListView {
            id: treeList
            property var nodeController: tree.controller
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 1
            model: treeModel

            delegate: Rectangle {
                id: treeDelegate
                required property string nodeType
                required property string nodeName
                required property string category
                width: treeList.width
                height: 38
                color: mouseArea.containsMouse ? Theme.nodeBgSelected
                                               : "transparent"

                Drag.dragType: Drag.Automatic
                Drag.active: treeDelegate.dragging
                Drag.supportedActions: Qt.CopyAction
                Drag.keys: ["qnodegraph.node"]
                Drag.mimeData: { "text/plain": nodeType }
                property bool dragging: false
                property bool dropHandled: false

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 6
                    spacing: 6

                    Label {
                        text: tree.categoryLabel(category)
                        color: Theme.textSecondary
                        Layout.preferredWidth: 62
                        elide: Text.ElideRight
                    }

                    Label {
                        text: nodeName
                        color: Theme.textPrimary
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    property point pressPoint

                    onPressed: function(mouse) {
                        pressPoint = Qt.point(mouse.x, mouse.y)
                        treeDelegate.dropHandled = false
                    }
                    onPositionChanged: function(mouse) {
                        if (!treeDelegate.dragging &&
                            (Math.abs(mouse.x - pressPoint.x) > 8 ||
                             Math.abs(mouse.y - pressPoint.y) > 8)) {
                            treeDelegate.dragging = true
                        }
                    }
                    onReleased: function(mouse) {
                        if (treeDelegate.dragging) {
                            treeDelegate.Drag.drop()
                            if (!treeDelegate.dropHandled && tree.dropTarget) {
                                var targetPoint = tree.dropTarget.mapFromItem(
                                            treeDelegate, mouse.x, mouse.y)
                                tree.dropTarget.controller.addNodeTypeAt(
                                            nodeType, targetPoint.x, targetPoint.y, false)
                            }
                            treeDelegate.dragging = false
                        } else {
                            treeList.nodeController.addNodeType(nodeType)
                        }
                    }
                }
            }
        }
    }
}
