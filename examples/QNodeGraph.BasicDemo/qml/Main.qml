import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QNodeGraph.UI 1.0

ApplicationWindow {
    id: root
    width: 1280
    height: 800
    visible: true
    title: qsTr("QNodeGraph Basic Demo")
    color: "#20252b"

    header: ToolBar {
        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12

            Label {
                text: qsTr("QNodeGraph library")
                color: "#f0f3f6"
                font.bold: true
            }

            Label {
                text: qsTr("Nodes: %1").arg(canvas.controller.nodeCount)
                color: "#aab4bf"
                Layout.fillWidth: true
            }

            Button {
                text: qsTr("Add demo node")
                onClicked: canvas.controller.addDemoNode()
            }

            Button {
                text: qsTr("Clear")
                onClicked: canvas.controller.clearGraph()
            }

            Button {
                text: qsTr("-")
                onClicked: canvas.zoomOut()
            }

            Button {
                text: qsTr("100%")
                onClicked: canvas.resetView()
            }

            Button {
                text: qsTr("+")
                onClicked: canvas.zoomIn()
            }
        }
    }

    GraphCanvas {
        id: canvas
        anchors.fill: parent
        anchors.margins: 16
    }
}
