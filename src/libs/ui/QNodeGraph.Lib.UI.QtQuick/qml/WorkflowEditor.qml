import QtQuick
import QtQuick.Layouts

RowLayout {
    id: editor

    property alias controller: graph.controller
    property alias pipeLayout: graph.pipeLayout
    property alias wireColor: graph.wireColor

    spacing: 6

    GraphCanvas {
        id: graph
        Layout.fillWidth: true
        Layout.fillHeight: true
        Layout.minimumWidth: 360
    }

    PropertiesBin {
        id: properties
        Layout.fillHeight: true
        Layout.preferredWidth: 250
        Layout.minimumWidth: 210
        controller: graph.controller
    }
}
