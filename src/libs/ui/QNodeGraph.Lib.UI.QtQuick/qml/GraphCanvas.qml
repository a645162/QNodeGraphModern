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

        onConnectionDataChanged: requestPaint()
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
                required property real nodeX
                required property real nodeY
                required property int inputPortCount
                required property int outputPortCount

                x: nodeItem.nodeX
                y: nodeItem.nodeY
                width: 180
                height: 108
                color: nodeItem.selected ? "#394b5d" : "#2a333d"
                border.color: nodeItem.selected ? "#69a7dc" : "#566575"
                border.width: nodeItem.selected ? 2 : 1
                radius: 4

                property bool selected: canvas.selectedIndex === nodeItem.index

                Rectangle {
                    width: parent.width
                    height: 28
                    color: nodeItem.selected ? "#426b91" : "#34414d"
                    radius: 4

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: nodeItem.nodeName
                        color: "#e6edf3"
                        font.bold: true
                    }
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("%1 -> %2").arg(nodeItem.inputPortCount)
                        .arg(nodeItem.outputPortCount)
                    color: "#9aa6b2"
                }

                Rectangle {
                    x: -6
                    y: height / 2 - 6
                    width: 12
                    height: 12
                    radius: 6
                    color: "#6fa8dc"
                    border.color: "#c4e2ff"
                }

                Rectangle {
                    x: parent.width - 6
                    y: height / 2 - 6
                    width: 12
                    height: 12
                    radius: 6
                    color: "#7bc58c"
                    border.color: "#d5f6dc"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: canvas.selectedIndex = nodeItem.index
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

    MouseArea {
        id: selectionArea
        anchors.fill: parent
        z: -1
        acceptedButtons: Qt.LeftButton
        onPressed: function(mouse) {
            canvas.selecting = true
            canvas.selectionStart = Qt.point(mouse.x, mouse.y)
            canvas.selectionEnd = canvas.selectionStart
            canvas.selectedIndex = -1
        }
        onPositionChanged: function(mouse) {
            if (canvas.selecting)
                canvas.selectionEnd = Qt.point(mouse.x, mouse.y)
        }
        onReleased: canvas.selecting = false
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
