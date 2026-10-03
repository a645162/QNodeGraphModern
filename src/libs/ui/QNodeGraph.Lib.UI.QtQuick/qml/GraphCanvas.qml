import QtQuick
import QtQuick.Controls
import QNodeGraph.UI 1.0

pragma ComponentBehavior: Bound

Rectangle {
    id: canvas
    color: "#171b20"
    border.color: "#3c4652"
    radius: 4

    property alias controller: graphController
    property real zoomFactor: 1.0
    property point panOffset: Qt.point(0, 0)
    property int selectedIndex: -1
    property bool selecting: false
    property point selectionStart: Qt.point(0, 0)
    property point selectionEnd: Qt.point(0, 0)
    property bool slicing: false
    property point sliceStart: Qt.point(0, 0)
    property point sliceEnd: Qt.point(0, 0)

    onZoomFactorChanged: {
        grid.requestPaint()
        pipes.requestPaint()
    }
    onPanOffsetChanged: {
        grid.requestPaint()
        pipes.requestPaint()
    }
    function clampZoom(value) {
        return Math.max(0.25, Math.min(2.5, value))
    }

    function zoomAt(factor, centerX, centerY) {
        var oldZoom = zoomFactor
        var newZoom = clampZoom(oldZoom * factor)
        if (newZoom === oldZoom)
            return
        var worldX = (centerX - panOffset.x) / oldZoom
        var worldY = (centerY - panOffset.y) / oldZoom
        zoomFactor = newZoom
        panOffset = Qt.point(centerX - worldX * newZoom,
                             centerY - worldY * newZoom)
    }

    function resetView() {
        zoomFactor = 1.0
        panOffset = Qt.point(0, 0)
    }

    function zoomIn() {
        zoomAt(1.15, width / 2, height / 2)
    }

    function zoomOut() {
        zoomAt(1 / 1.15, width / 2, height / 2)
    }

    GraphController {
        id: graphController
    }

    Menu {
        id: contextMenu

        MenuItem {
            text: qsTr("Add Grayscale Node")
            onTriggered: canvas.controller.addNodeType("grayscale")
        }

        MenuItem {
            text: qsTr("Auto Layout")
            onTriggered: canvas.controller.autoLayout()
        }

        MenuItem {
            text: qsTr("Clear Graph")
            onTriggered: canvas.controller.clearGraph()
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Reset View")
            onTriggered: canvas.resetView()
        }
    }

    Connections {
        target: graphController
        function onSelectedRowChanged() {
            if (canvas.selectedIndex !== graphController.selectedRow)
                canvas.selectedIndex = graphController.selectedRow
        }
    }

    Canvas {
        id: grid
        anchors.fill: parent
        renderTarget: Canvas.FramebufferObject
        onPaint: {
            var context = getContext("2d")
            context.reset()
            context.fillStyle = "#171b20"
            context.fillRect(0, 0, width, height)
            var spacing = 24 * canvas.zoomFactor
            var startX = ((canvas.panOffset.x % spacing) + spacing) % spacing
            var startY = ((canvas.panOffset.y % spacing) + spacing) % spacing
            context.strokeStyle = "#252d36"
            context.lineWidth = 1
            for (var x = startX; x < width; x += spacing) {
                context.beginPath()
                context.moveTo(x, 0)
                context.lineTo(x, height)
                context.stroke()
            }
            for (var y = startY; y < height; y += spacing) {
                context.beginPath()
                context.moveTo(0, y)
                context.lineTo(width, y)
                context.stroke()
            }
        }
    }

    Canvas {
        id: pipes
        anchors.fill: parent
        z: -1
        property var connectionData: graphController.connections
        property var previewData: graphController.connectionPreview

        onConnectionDataChanged: requestPaint()
        onPreviewDataChanged: requestPaint()
        onPaint: {
            var context = getContext("2d")
            context.reset()
            context.strokeStyle = "#7d9bb8"
            context.lineWidth = 3
            for (var i = 0; i < connectionData.length; ++i) {
                var connection = connectionData[i]
                var startX = canvas.panOffset.x +
                             (connection.outputX + connection.outputWidth) * canvas.zoomFactor
                var startY = canvas.panOffset.y + connection.outputY * canvas.zoomFactor
                var endX = canvas.panOffset.x + connection.inputX * canvas.zoomFactor
                var endY = canvas.panOffset.y + connection.inputY * canvas.zoomFactor
                var distance = Math.max(40, Math.abs(endX - startX) * 0.5)
                context.beginPath()
                context.moveTo(startX, startY)
                context.bezierCurveTo(startX + distance, startY,
                                      endX - distance, endY, endX, endY)
                context.stroke()
            }

            if (previewData && Object.keys(previewData).length > 0) {
                var previewStartX = canvas.panOffset.x +
                                    (previewData.outputX + previewData.outputWidth) *
                                    canvas.zoomFactor
                var previewStartY = canvas.panOffset.y +
                                    previewData.outputY * canvas.zoomFactor
                var previewEndX = canvas.panOffset.x +
                                  previewData.inputX * canvas.zoomFactor
                var previewEndY = canvas.panOffset.y +
                                  previewData.inputY * canvas.zoomFactor
                var previewDistance = Math.max(
                    40, Math.abs(previewEndX - previewStartX) * 0.5)
                context.strokeStyle = "#c8d9e8"
                context.lineWidth = 2
                context.setLineDash([7, 5])
                context.beginPath()
                context.moveTo(previewStartX, previewStartY)
                context.bezierCurveTo(previewStartX + previewDistance,
                                      previewStartY,
                                      previewEndX - previewDistance,
                                      previewEndY, previewEndX, previewEndY)
                context.stroke()
                context.setLineDash([])
            }
        }
    }

    Item {
        id: world
        x: canvas.panOffset.x
        y: canvas.panOffset.y
        scale: canvas.zoomFactor

        Repeater {
            model: graphController

            delegate: Rectangle {
                id: nodeItem
                required property int index
                required property string nodeName
                required property string nodeType
                required property string nodeIcon
                required property string nodeAccent
                required property bool nodeEnabled
                required property string nodeLabel
                required property real nodeWidth
                required property real nodeHeight
                required property string nodeColor
                required property bool nodeIsBackdrop
                required property bool nodeIsGroup
                required property real nodeX
                required property real nodeY
                required property int inputPortCount
                required property int outputPortCount

                x: nodeItem.nodeX
                y: nodeItem.nodeY
                width: nodeItem.nodeWidth
                height: nodeItem.nodeHeight
                z: nodeItem.nodeIsBackdrop ? -0.5 : 0
                color: nodeItem.nodeIsBackdrop
                       ? nodeItem.nodeColor
                       : (nodeItem.selected ? "#394b5d" : "#2a333d")
                border.color: nodeItem.selected
                              ? "#69a7dc"
                              : (nodeItem.nodeIsBackdrop ? nodeItem.nodeAccent
                                                         : "#566575")
                border.width: nodeItem.selected ? 2 : 1
                radius: 4
                opacity: nodeItem.nodeEnabled ? 1.0 : 0.55

                property bool selected: canvas.selectedIndex === nodeItem.index

                Rectangle {
                    width: parent.width
                    height: 28
                    color: nodeItem.nodeIsBackdrop
                           ? nodeItem.nodeAccent
                           : nodeItem.nodeAccent
                    radius: 4

                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        width: 28
                        height: 20
                        radius: 3
                        color: "#26000000"

                        Label {
                            anchors.centerIn: parent
                            text: nodeItem.nodeIcon
                            color: "#f1f6fa"
                            font.pixelSize: 9
                            font.bold: true
                        }
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 40
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        text: nodeItem.nodeLabel
                        color: "#e6edf3"
                        font.bold: true
                        elide: Text.ElideRight
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: nodeItem.nodeIsBackdrop
                          ? qsTr("Backdrop")
                          : (nodeItem.nodeIsGroup
                             ? qsTr("Group  %1 -> %2").arg(
                                   nodeItem.inputPortCount).arg(
                                   nodeItem.outputPortCount)
                             : qsTr("%1 -> %2").arg(nodeItem.inputPortCount)
                               .arg(nodeItem.outputPortCount))
                    color: "#9aa6b2"
                }

                Rectangle {
                    x: -6
                    y: nodeItem.height / 2 - 6
                    width: 12
                    height: 12
                    radius: 6
                    color: "#6fa8dc"
                    border.color: "#c4e2ff"

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        onPressed: {
                            mouse.accepted = true
                            canvas.controller.beginConnection(nodeItem.index)
                        }
                        onPositionChanged: {
                            var point = parent.mapToItem(canvas, mouse.x, mouse.y)
                            canvas.controller.updateConnectionPreview(
                                (point.x - canvas.panOffset.x) /
                                canvas.zoomFactor,
                                (point.y - canvas.panOffset.y) /
                                canvas.zoomFactor)
                        }
                        onReleased: {
                            var point = parent.mapToItem(canvas, mouse.x, mouse.y)
                            var worldX = (point.x - canvas.panOffset.x) /
                                         canvas.zoomFactor
                            var worldY = (point.y - canvas.panOffset.y) /
                                         canvas.zoomFactor
                            if (!canvas.controller.completeConnectionAt(worldX,
                                                                         worldY))
                                canvas.controller.cancelConnection()
                        }
                    }
                }

                Rectangle {
                    x: parent.width - 6
                    y: nodeItem.height / 2 - 6
                    width: 12
                    height: 12
                    radius: 6
                    color: canvas.controller.connectionPending ? "#f0b45f" : "#7bc58c"
                    border.color: "#d5f6dc"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        canvas.selectedIndex = nodeItem.index
                        canvas.controller.selectNode(nodeItem.index)
                    }
                }

                CheckBox {
                    id: enabledToggle
                    z: 3
                    anchors.right: parent.right
                    anchors.rightMargin: 5
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 3
                    text: qsTr("Enabled")
                    checked: nodeItem.nodeEnabled
                    scale: 0.72
                    transformOrigin: Item.BottomRight
                    onToggled: canvas.controller.setNodeProperty(
                        nodeItem.index, "enabled", checked)
                }

                DragHandler {
                    acceptedButtons: Qt.LeftButton
                    target: null
                    property point startPosition: Qt.point(0, 0)
                    onActiveChanged: {
                        if (active)
                            startPosition = Qt.point(nodeItem.nodeX, nodeItem.nodeY)
                    }
                    onTranslationChanged: {
                        if (active)
                            canvas.controller.moveNode(
                                nodeItem.index,
                                startPosition.x + translation.x / canvas.zoomFactor,
                                startPosition.y + translation.y / canvas.zoomFactor)
                    }
                }
            }
        }
    }

    Rectangle {
        visible: canvas.selecting
        x: Math.min(canvas.selectionStart.x, canvas.selectionEnd.x)
        y: Math.min(canvas.selectionStart.y, canvas.selectionEnd.y)
        width: Math.abs(canvas.selectionEnd.x - canvas.selectionStart.x)
        height: Math.abs(canvas.selectionEnd.y - canvas.selectionStart.y)
        color: "#37648840"
        border.color: "#69a7dc"
        border.width: 1
    }

    Canvas {
        id: sliceGuide
        anchors.fill: parent
        visible: canvas.slicing
        z: 2
        onVisibleChanged: requestPaint()
        onPaint: {
            var context = getContext("2d")
            context.reset()
            context.strokeStyle = "#efb168"
            context.lineWidth = 2
            context.setLineDash([8, 5])
            context.beginPath()
            context.moveTo(canvas.sliceStart.x, canvas.sliceStart.y)
            context.lineTo(canvas.sliceEnd.x, canvas.sliceEnd.y)
            context.stroke()
            context.setLineDash([])
        }
    }

    MouseArea {
        id: selectionArea
        anchors.fill: parent
        z: -1
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onPressed: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                contextMenu.x = mouse.x
                contextMenu.y = mouse.y
                contextMenu.open()
                mouse.accepted = true
                return
            }
            if ((mouse.modifiers & Qt.ShiftModifier) !== 0) {
                canvas.slicing = true
                canvas.sliceStart = Qt.point(mouse.x, mouse.y)
                canvas.sliceEnd = canvas.sliceStart
                sliceGuide.requestPaint()
                mouse.accepted = true
                return
            }
            canvas.selecting = true
            canvas.selectionStart = Qt.point(mouse.x, mouse.y)
            canvas.selectionEnd = canvas.selectionStart
            canvas.selectedIndex = -1
            canvas.controller.selectNode(-1)
        }
        onPositionChanged: function(mouse) {
            if (canvas.slicing) {
                canvas.sliceEnd = Qt.point(mouse.x, mouse.y)
                sliceGuide.requestPaint()
                return
            }
            if (canvas.selecting)
                canvas.selectionEnd = Qt.point(mouse.x, mouse.y)
        }
        onReleased: function(mouse) {
            if (canvas.slicing) {
                canvas.controller.sliceConnections(
                    (canvas.sliceStart.x - canvas.panOffset.x) /
                    canvas.zoomFactor,
                    (canvas.sliceStart.y - canvas.panOffset.y) /
                    canvas.zoomFactor,
                    (canvas.sliceEnd.x - canvas.panOffset.x) /
                    canvas.zoomFactor,
                    (canvas.sliceEnd.y - canvas.panOffset.y) /
                    canvas.zoomFactor)
                canvas.slicing = false
                sliceGuide.requestPaint()
                mouse.accepted = true
                return
            }
            canvas.selecting = false
        }
    }

    WheelHandler {
        onWheel: function(event) {
            canvas.zoomAt(event.angleDelta.y > 0 ? 1.1 : 1 / 1.1,
                          point.position.x, point.position.y)
            event.accepted = true
        }
    }

    DragHandler {
        id: panHandler
        acceptedButtons: Qt.MiddleButton
        target: null
        property point startPan: Qt.point(0, 0)
        onActiveChanged: {
            if (active)
                startPan = canvas.panOffset
        }
        onTranslationChanged: {
            if (active)
                canvas.panOffset = Qt.point(startPan.x + translation.x,
                                            startPan.y + translation.y)
        }
    }
}
