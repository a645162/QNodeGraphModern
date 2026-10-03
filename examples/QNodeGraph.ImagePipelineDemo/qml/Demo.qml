import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QNodeGraph.UI 1.0

ApplicationWindow {
    visible: true
    width: 1100
    height: 700
    title: qsTr("QNodeGraph Image Pipeline Demo")
    color: "#1b2026"

    GraphController { id: controller }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 24
        spacing: 16

        Label {
            text: qsTr("Image processing pipeline")
            color: "#f0f3f6"
            font.pixelSize: 26
        }

        Label {
            text: qsTr("Scaffold: Load Image -> Process -> Preview")
            color: "#9aa6b2"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            Repeater {
                model: [qsTr("Load Image"), qsTr("Grayscale"), qsTr("Preview")]

                delegate: Rectangle {
                    Layout.preferredWidth: 240
                    Layout.preferredHeight: 150
                    color: "#2a313a"
                    border.color: "#566575"
                    radius: 5

                    Column {
                        anchors.centerIn: parent
                        spacing: 8

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData
                            color: "#e6edf3"
                            font.bold: true
                        }

                        Label {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: qsTr("Image node")
                            color: "#9aa6b2"
                        }
                    }
                }
            }
        }

        Button {
            text: qsTr("Add demo node (%1)").arg(controller.nodeCount)
            onClicked: controller.addDemoNode()
        }
    }
}
