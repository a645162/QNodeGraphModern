import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: editor

    property alias controller: graph.controller
    property alias pipeLayout: graph.pipeLayout
    property alias wireColor: graph.wireColor
    // 节点面板模型：宿主用它安装自己的算子注册表（否则面板里没有业务算子）。
    property alias toolboxModel: palette.nodePaletteModel
    // WorkflowPanel injects the QWidget bridge. The standalone probe leaves
    // this null and keeps the canvas usable without application settings.
    property var workflowHost: typeof workflowPanel === "undefined"
                                ? null : workflowPanel
    property var helpProvider: typeof workflowHelp === "undefined"
                               ? null : workflowHelp

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
            helpProvider: editor.helpProvider
        }

        GraphCanvas {
            id: graph
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 360
            helpProvider: editor.helpProvider
        }

        PropertiesBin {
            id: properties
            Layout.fillHeight: true
            Layout.preferredWidth: collapsed ? 30 : 250
            Layout.minimumWidth: collapsed ? 30 : 210
            controller: graph.controller
            helpProvider: editor.helpProvider
        }
    }

    Dialog {
        id: helpDialog
        modal: false
        title: editor.helpProvider && editor.helpProvider.typeId.length > 0
               ? qsTr("帮助: %1").arg(editor.helpProvider.typeId)
               : qsTr("工作流帮助")
        width: Math.min(editor.width * 0.7, 720)
        height: Math.min(editor.height * 0.8, 560)
        visible: editor.helpProvider ? editor.helpProvider.open : false
        standardButtons: Dialog.Close

        onClosed: {
            if (editor.helpProvider)
                editor.helpProvider.close()
        }

        contentItem: ScrollView {
            clip: true
            Text {
                width: helpDialog.availableWidth
                text: editor.helpProvider ? editor.helpProvider.markdown : ""
                textFormat: Text.MarkdownText
                wrapMode: Text.WordWrap
                color: Theme.textPrimary
                padding: 12
            }
        }
    }
}
