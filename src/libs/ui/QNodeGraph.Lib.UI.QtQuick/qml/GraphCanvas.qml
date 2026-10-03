import QtQuick
import QtQuick.Controls
import QNodeGraph.UI 1.0

Rectangle {
    id: canvas
    color: "#171b20"
    border.color: "#3c4652"
    radius: 4

    property alias controller: graphController

    GraphController {
        id: graphController
    }

    Label {
        anchors.centerIn: parent
        text: qsTr("QNodeGraph canvas library")
        color: "#aab4bf"
    }
}
