import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: editor

    property alias controller: graph.controller
    property alias pipeLayout: graph.pipeLayout
    property alias wireColor: graph.wireColor
    // WorkflowPanel injects the QWidget bridge. The standalone probe leaves
    // this null and keeps the canvas usable without application settings.
    property var workflowHost: typeof workflowPanel === "undefined"
                                ? null : workflowPanel

    spacing: 5

    RowLayout {
        Layout.fillWidth: true
        Layout.preferredHeight: 34
        spacing: 10

        Label {
            text: qsTr("图像处理工作流")
            color: Theme.textPrimary
            font.bold: true
        }

        CheckBox {
            id: workflowEnabledToggle
            text: qsTr("启用")
            checked: editor.workflowHost
                     ? editor.workflowHost.workflowEnabled : true
            onToggled: {
                if (editor.workflowHost)
                    editor.workflowHost.workflowEnabled = checked
            }
        }

        Item { Layout.fillWidth: true }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 6

        NodesPalette {
            id: palette
            Layout.fillHeight: true
            Layout.preferredWidth: collapsed ? 30 : 230
            Layout.minimumWidth: collapsed ? 30 : 190
            controller: graph.controller
            dropTarget: graph
        }

        GraphCanvas {
            id: graph
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 360
        }

        PropertiesBin {
            id: properties
            Layout.fillHeight: true
            Layout.preferredWidth: collapsed ? 30 : 250
            Layout.minimumWidth: collapsed ? 30 : 210
            controller: graph.controller
        }
    }
}
