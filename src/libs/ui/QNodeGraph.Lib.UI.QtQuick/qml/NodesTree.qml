import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: tree

    required property var controller
    color: "#20262d"
    border.color: "#3c4652"
    radius: 4

    NodePaletteModel {
        id: treeModel
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        Label {
            text: qsTr("Nodes Tree")
            color: "#f0f3f6"
            font.bold: true
            font.pixelSize: 16
        }

        ListView {
            id: treeList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 1
            model: treeModel

            delegate: Rectangle {
                required property string nodeType
                required property string nodeName
                required property string category
                width: treeList.width
                height: 38
                color: mouseArea.containsMouse ? "#344452" : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 6
                    spacing: 6

                    Label {
                        text: category
                        color: "#738292"
                        Layout.preferredWidth: 62
                        elide: Text.ElideRight
                    }

                    Label {
                        text: nodeName
                        color: "#d7e0e8"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                }

                MouseArea {
                    id: mouseArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: tree.controller.addNodeType(nodeType)
                }
            }
        }
    }
}
