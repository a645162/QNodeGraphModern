import QtQuick
import QtQuick.Shapes

Item {
    id: root

    property var connections: []
    property var preview: ({})
    property point panOffset: Qt.point(0, 0)
    property real zoomFactor: 1.0
    property int layoutMode: 0
    property bool previewActive: false

    Repeater {
        model: root.connections

        delegate: Shape {
            required property var modelData
            anchors.fill: parent
            z: 0
            antialiasing: true

            ShapePath {
                fillColor: "transparent"
                strokeColor: Theme.wire
                strokeWidth: 2
                capStyle: ShapePath.RoundCap
                joinStyle: ShapePath.RoundJoin
                PathPolyline {
                    path: root.pathFor(modelData)
                }
            }
        }
    }

    Shape {
        anchors.fill: parent
        z: 1
        antialiasing: true
        visible: root.preview && root.preview.inputX !== undefined

        ShapePath {
            fillColor: "transparent"
            strokeColor: root.previewActive ? Theme.wireActive : Theme.wire
            strokeWidth: 2
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathPolyline {
                path: root.preview && root.preview.inputX !== undefined
                      ? root.pathFor(root.preview)
                      : []
            }
        }
    }

    function screenPoint(x, y) {
        return Qt.point(root.panOffset.x + x * root.zoomFactor,
                        root.panOffset.y + y * root.zoomFactor)
    }

    function pathFor(connection) {
        if (!connection || connection.outputX === undefined)
            return []
        var start = screenPoint(connection.outputX + connection.outputWidth,
                                connection.outputY)
        var end = screenPoint(connection.inputX, connection.inputY)
        if (root.layoutMode === 2)
            return [start, end]
        if (root.layoutMode === 1) {
            var middleX = (start.x + end.x) * 0.5
            return [start, Qt.point(middleX, start.y),
                    Qt.point(middleX, end.y), end]
        }
        var distance = Math.max(40.0, Math.abs(end.x - start.x) * 0.5)
        var controlStart = Qt.point(start.x + distance * root.zoomFactor,
                                    start.y)
        var controlEnd = Qt.point(end.x - distance * root.zoomFactor, end.y)
        var points = []
        var samples = 24
        for (var index = 0; index <= samples; ++index) {
            var t = index / samples
            var inverse = 1.0 - t
            var x = inverse * inverse * inverse * start.x +
                    3.0 * inverse * inverse * t * controlStart.x +
                    3.0 * inverse * t * t * controlEnd.x +
                    t * t * t * end.x
            var y = inverse * inverse * inverse * start.y +
                    3.0 * inverse * inverse * t * controlStart.y +
                    3.0 * inverse * t * t * controlEnd.y +
                    t * t * t * end.y
            points.push(Qt.point(x, y))
        }
        return points
    }
}
