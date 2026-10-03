import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
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
                text: qsTr("Open")
                onClicked: openDialog.open()
            }

            Button {
                text: qsTr("Save")
                onClicked: saveDialog.open()
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
                text: qsTr("Undo")
                enabled: canvas.controller.canUndo
                onClicked: canvas.controller.undo()
            }

            Button {
                text: qsTr("Redo")
                enabled: canvas.controller.canRedo
                onClicked: canvas.controller.redo()
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
                text: qsTr("Fit")
                onClicked: canvas.fitToNodes()
            }

            Button {
                text: qsTr("+")
                onClicked: canvas.zoomIn()
            }

            ComboBox {
                model: [qsTr("Curved"), qsTr("Angled"), qsTr("Straight")]
                currentIndex: canvas.pipeLayout
                onActivated: canvas.pipeLayout = currentIndex
                implicitWidth: 110
            }

            Label {
                visible: canvas.controller.lastError.length > 0
                text: canvas.controller.lastError
                color: "#ef9a9a"
                elide: Text.ElideRight
                Layout.maximumWidth: 280
            }
        }
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open Node Graph")
        fileMode: FileDialog.OpenFile
        nameFilters: [qsTr("Node Graph (*.json)"), qsTr("All Files (*)")]
        onAccepted: canvas.controller.loadGraph(
                        selectedFile.toString().replace("file:///", ""))
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save Node Graph")
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: [qsTr("Node Graph (*.json)"), qsTr("All Files (*)")]
        onAccepted: canvas.controller.saveGraph(
                        selectedFile.toString().replace("file:///", ""))
    }

    Shortcut {
        sequence: "Ctrl+Z"
        onActivated: canvas.controller.undo()
    }

    Shortcut {
        sequence: "Ctrl+Y"
        onActivated: canvas.controller.redo()
    }

    Shortcut {
        sequence: "Ctrl+P"
        onActivated: tabSearch.open()
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        ColumnLayout {
            Layout.preferredWidth: 230
            Layout.fillHeight: true
            spacing: 8

            TabBar {
                id: nodeTabs
                Layout.fillWidth: true
                TabButton { text: qsTr("Palette") }
                TabButton { text: qsTr("Tree") }
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: nodeTabs.currentIndex

                NodesPalette { controller: canvas.controller }
                NodesTree { controller: canvas.controller }
            }
        }

        GraphCanvas {
            id: canvas
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        PropertiesBin {
            Layout.preferredWidth: 260
            Layout.fillHeight: true
            controller: canvas.controller
        }
    }

    TabSearch {
        id: tabSearch
        anchors.centerIn: parent
        controller: canvas.controller
    }
}
