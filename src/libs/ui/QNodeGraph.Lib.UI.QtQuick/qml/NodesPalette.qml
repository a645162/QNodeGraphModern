import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: nodesPalette

    required property var controller
    property Item dropTarget: null
    color: Theme.panelBg
    border.color: Theme.panelBorder
    radius: Theme.radius

    NodePaletteModel {
        id: paletteModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: qsTr("Nodes Palette")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: 16
        }

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Search nodes")
            selectByMouse: true
            onTextChanged: paletteModel.filter = text
        }

        ListView {
            id: nodeList
            property var nodeController: nodesPalette.controller
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 4
            model: paletteModel

            delegate: Rectangle {
                id: paletteDelegate
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
