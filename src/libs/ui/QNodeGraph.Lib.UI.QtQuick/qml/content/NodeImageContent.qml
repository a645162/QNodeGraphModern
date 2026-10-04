import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: customContent

    // Injected by GraphCanvas when the Loader instantiates this component.
    property int nodeRow: -1
    property var nodeController: null

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 4
        spacing: 4

        Label {
            text: qsTr("Blur Radius")
            color: Theme.nodeBodyText
            font.pixelSize: 11
        }

        Slider {
            id: radiusSlider
            Layout.fillWidth: true
            from: 0
            to: 40
            value: 8
            onMoved: {
                if (customContent.nodeController &&
                    customContent.nodeRow >= 0)
                    customContent.nodeController.setNodeProperty(
                        customContent.nodeRow, "radius", value)
            }
        }

        Image {
            Layout.fillWidth: true
            Layout.fillHeight: true
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            source: customContent.nodeRow >= 0 && customContent.nodeController
                    ? customContent.nodeController.index(
                          customContent.nodeRow, 0)
                      .data(GraphController.NodePreviewSourceRole)
                    : ""
        }
    }
}
