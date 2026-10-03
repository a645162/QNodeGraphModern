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
            text: qsTr("Load -> Grayscale -> Edge Detect -> Preview -> Save")
            color: "#9aa6b2"
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            Repeater {
                model: [qsTr("Load Image"), qsTr("Grayscale"),
                    qsTr("Edge Detect"), qsTr("Preview"), qsTr("Save Image")]

                delegate: Rectangle {
                    required property string modelData
                    property bool isPreview: modelData === qsTr("Preview")
                    property bool isSave: modelData === qsTr("Save Image")
                    Layout.preferredWidth: 190
                    Layout.preferredHeight: 178
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
                            text: isPreview
                                  ? qsTr("Preview node")
                                  : isSave ? qsTr("Output node") : qsTr("Image node")
                            color: "#9aa6b2"
                        }

                        Image {
                            visible: isPreview && pipelineController.previewUrl.length > 0
                            width: 148
                            height: 92
                            fillMode: Image.PreserveAspectFit
                            source: pipelineController.previewUrl
                        }

                        Label {
                            visible: isPreview
                            width: 170
                            horizontalAlignment: Text.AlignHCenter
                            text: pipelineController.imageWidth > 0
                                  ? qsTr("%1 x %2, %3 channel")
                                    .arg(pipelineController.imageWidth)
                                    .arg(pipelineController.imageHeight)
                                    .arg(pipelineController.imageChannels)
                                  : qsTr("Waiting for image")
                            color: "#b8c5d1"
                            elide: Text.ElideRight
                        }

                        Label {
                            visible: isSave
                            width: 170
                            horizontalAlignment: Text.AlignHCenter
                            text: pipelineController.outputPath.length > 0
                                  ? qsTr("Saved PNG") : qsTr("Waiting for output")
                            color: "#b8c5d1"
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 260
            color: "#14191e"
            border.color: "#566575"
            radius: 5

            Image {
                anchors.fill: parent
                anchors.margins: 12
                source: pipelineController.previewUrl
                fillMode: Image.PreserveAspectFit
                visible: source.length > 0
            }

            Label {
                anchors.centerIn: parent
                text: qsTr("Run the pipeline to display the processed image")
                color: "#738292"
                visible: pipelineController.previewUrl.length === 0
            }
        }

        Button {
            text: pipelineController.processing
                  ? qsTr("Processing...")
                  : qsTr("Run image pipeline")
            enabled: !pipelineController.processing
            onClicked: pipelineController.runDemo()
        }

        Button {
            text: qsTr("Cancel")
            enabled: pipelineController.processing
            onClicked: pipelineController.cancel()
        }

        Label {
            text: pipelineController.status
            color: "#aab4bf"
        }
    }
}
