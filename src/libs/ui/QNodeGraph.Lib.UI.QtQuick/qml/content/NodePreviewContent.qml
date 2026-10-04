import QtQuick
import QtQuick.Controls

Item {
    id: previewContent

    // GraphCanvas injects these properties when a descriptor supplies custom
    // content. The host updates NodePreviewSourceRole through ImageFrameProvider.
    property int nodeRow: -1
    property var nodeController: null

    Label {
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        text: qsTr("Image Viewer")
        color: Theme.nodeBodyText
        font.pixelSize: 11
    }

    Image {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.margins: 4
        anchors.topMargin: 20
        fillMode: Image.PreserveAspectFit
        // ImageFrameProvider returns a small in-memory QImage. Loading it
        // synchronously keeps revision-based preview updates deterministic.
        asynchronous: false
        cache: false
        source: previewContent.nodeRow >= 0 && previewContent.nodeController
                    ? previewContent.nodeController.index(
                          previewContent.nodeRow, 0)
                          .data(GraphController.NodePreviewSourceRole)
                    : ""
    }
}
