import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QNodeGraph.UI 1.0

ApplicationWindow {
    visible: true
    width: 1400
    height: 900
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

        GraphCanvas {
            id: graphCanvas
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.preferredHeight: 320
            Layout.minimumHeight: 180

            // 节点必须接线：未参与有效链路的节点会被画布按「剪枝」降到 0.38
            // 不透明度，看起来像整张流程图消失了。
            Component.onCompleted: {
                controller.addNodeType("load_image", false)
                controller.addNodeType("grayscale", true)
                controller.addNodeType("edge_detect", true)
                controller.addNodeType("image_preview", true)
                // Save Image 是汇点且没有来源端口可接（上一个是 Image
                // Preview，只有输入端），autoLayout 会把它排到第一列的第二
                // 行，超出画布可视区。稍后统一 fitToNodes() 缩放到全部可见。
                controller.addNodeType("save_image")
                controller.autoLayout()
            }
        }

        // Component.onCompleted 时画布还没拿到最终尺寸，fitToNodes() 会按
        // 0x0 视口算缩放；延到下一次事件循环再适配一次。
        Timer {
            interval: 0
            running: true
            repeat: false
            onTriggered: graphCanvas.fitToNodes()
        }

        Connections {
            target: pipelineController
            function onPreviewUrlChanged() {
                if (pipelineController.previewUrl.length > 0)
                    graphCanvas.controller.setNodePreview(
                        3, pipelineController.previewUrl,
                        pipelineController.imageWidth,
                        pipelineController.imageHeight,
                        pipelineController.imageChannels)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 178
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
            Layout.fillHeight: true
            Layout.preferredHeight: 260
            Layout.minimumHeight: 120
            color: "#14191e"
            border.color: "#566575"
            radius: 5

            Image {
                anchors.fill: parent
                anchors.margins: 12
                source: pipelineController.previewUrl
                fillMode: Image.PreserveAspectFit
                // Image.source 是 QUrl，source.length 恒为 undefined；必须用
                // 控制器上的字符串属性判断，否则大图区域永远不显示。
                visible: pipelineController.previewUrl.length > 0
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
