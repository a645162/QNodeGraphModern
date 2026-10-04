import QtQuick

Canvas {
    id: root

    property var connections: []
    property var preview: ({})
    property point panOffset: Qt.point(0, 0)
    property real zoomFactor: 1.0
    property int layoutMode: 0
    property bool previewActive: false

    renderTarget: Canvas.Image
    antialiasing: true

    onConnectionsChanged: requestPaint()
    onPreviewChanged: requestPaint()
    onPanOffsetChanged: requestPaint()
    onZoomFactorChanged: requestPaint()
    onLayoutModeChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    function screenPoint(x, y) {
        return Qt.point(root.panOffset.x + x * root.zoomFactor,
                        root.panOffset.y + y * root.zoomFactor)
    }

    function tracePath(context, connection) {
        if (!connection || connection.outputX === undefined)
            return false
        var start = screenPoint(connection.outputX + connection.outputWidth,
                                connection.outputY)
        var end = screenPoint(connection.inputX, connection.inputY)
        context.beginPath()
        if (root.layoutMode === 2) {
            context.moveTo(start.x, start.y)
            context.lineTo(end.x, end.y)
            return true
        }
        if (root.layoutMode === 1) {
            var middleX = (start.x + end.x) * 0.5
            context.moveTo(start.x, start.y)
            context.lineTo(middleX, start.y)
            context.lineTo(middleX, end.y)
            context.lineTo(end.x, end.y)
            return true
        }
        var distance = Math.max(40.0, Math.abs(end.x - start.x) * 0.5) *
                       root.zoomFactor
        context.moveTo(start.x, start.y)
        context.bezierCurveTo(start.x + distance, start.y,
                              end.x - distance, end.y, end.x, end.y)
        return true
    }

    onPaint: {
        var context = getContext("2d")
        context.reset()
        context.lineWidth = 2
        context.lineCap = "round"
        context.lineJoin = "round"

        context.strokeStyle = Theme.wire
        for (var index = 0; index < root.connections.length; ++index) {
            if (tracePath(context, root.connections[index]))
                context.stroke()
        }

        if (root.preview && root.preview.inputX !== undefined) {
            context.strokeStyle = root.previewActive ? Theme.wireActive
                                                     : Theme.wire
            if (tracePath(context, root.preview))
                context.stroke()
        }
    }
}
