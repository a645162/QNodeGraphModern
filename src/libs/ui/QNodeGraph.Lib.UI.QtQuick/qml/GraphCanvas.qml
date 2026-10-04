import QtQuick
import QtQuick.Controls
import QNodeGraph.UI 1.0

pragma ComponentBehavior: Bound

Rectangle {
    id: canvas
    focus: true
    color: Theme.canvasBg
    border.color: Theme.panelBorder
    radius: Theme.radius

    property alias controller: graphController
    readonly property bool darkTheme: Theme.dark
    function setThemeMode(value) {
        Theme.mode = value
        grid.requestPaint()
    }
    property real zoomFactor: 1.0
    property int pipeLayout: 0
    property point panOffset: Qt.point(0, 0)
    property int selectedIndex: -1
    property bool selecting: false
    property point selectionStart: Qt.point(0, 0)
    property point selectionEnd: Qt.point(0, 0)
    property bool selectionAdditive: false
    property bool slicing: false
    property point sliceStart: Qt.point(0, 0)
    property point sliceEnd: Qt.point(0, 0)
    property int contextNodeRow: -1
    property bool nodeDropActive: nodeDropArea.containsDrag

    onZoomFactorChanged: {
        grid.requestPaint()
    }
    onPanOffsetChanged: {
        grid.requestPaint()
    }
    Connections {
        target: Theme
        function onDarkChanged() {
            grid.requestPaint()
        }
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

    function fitToNodes() {
        var bounds = canvas.controller.graphBounds()
        if (!bounds || bounds.width === undefined) {
            resetView()
            return
        }
        var padding = 48
        var availableWidth = Math.max(1, width - padding * 2)
        var availableHeight = Math.max(1, height - padding * 2)
        var fittedZoom = clampZoom(Math.min(availableWidth / bounds.width,
                                             availableHeight / bounds.height))
        zoomFactor = fittedZoom
        panOffset = Qt.point((width - bounds.width * fittedZoom) / 2 -
                             bounds.x * fittedZoom,
                             (height - bounds.height * fittedZoom) / 2 -
                             bounds.y * fittedZoom)
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

    DropArea {
        id: nodeDropArea
        anchors.fill: parent
        keys: ["qnodegraph.node"]
        z: 4

        onDropped: function(drop) {
            var typeId = drop.getDataAsString("text/plain")
            if (typeId.length === 0 && drop.source)
                typeId = drop.source.nodeType
            if (typeId.length === 0)
                return

            var worldX = (drop.x - canvas.panOffset.x) / canvas.zoomFactor
            var worldY = (drop.y - canvas.panOffset.y) / canvas.zoomFactor
            if (canvas.controller.addNodeTypeAt(typeId, worldX, worldY,
                                                false)) {
                if (drop.source && drop.source.dropHandled !== undefined)
                    drop.source.dropHandled = true
                drop.acceptProposedAction()
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        color: "transparent"
        border.color: Theme.nodeBorderSelected
        border.width: 2
        radius: canvas.radius
        visible: canvas.nodeDropActive
        z: 5
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

    Menu {
        id: nodeContextMenu

        Menu {
            title: qsTr("Assign to Group")

            Repeater {
                model: graphController

                delegate: MenuItem {
                    required property int index
                    required property string nodeName
                    required property bool nodeIsGroup
                    visible: nodeIsGroup && index !== canvas.contextNodeRow
                    text: nodeName
                    onTriggered: graphController.assignNodeToGroup(
                                     canvas.contextNodeRow, index)
                }
            }
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("Delete Node")
            enabled: canvas.contextNodeRow >= 0 &&
                     !(graphController.index(canvas.contextNodeRow, 0)
                       .data(GraphController.NodeFixedRole))
            onTriggered: {
                if (canvas.contextNodeRow >= 0)
                    canvas.controller.deleteNode(canvas.contextNodeRow)
            }
        }

        MenuItem {
            text: qsTr("Remove from Group")
            enabled: canvas.contextNodeRow >= 0
            onTriggered: {
                if (canvas.contextNodeRow >= 0)
                    graphController.clearNodeGroup(canvas.contextNodeRow)
            }
        }

        MenuItem {
            text: qsTr("Clear Selection")
            onTriggered: {
                canvas.contextNodeRow = -1
                canvas.selectedIndex = -1
                canvas.controller.selectNode(-1)
            }
        }
    }

    Shortcut {
        sequence: "Delete"
        onActivated: {
            if (canvas.selectedIndex >= 0)
                canvas.controller.deleteNode(canvas.selectedIndex)
        }
    }

    Shortcut {
        sequence: "Backspace"
        onActivated: {
            if (canvas.selectedIndex >= 0)
                canvas.controller.deleteNode(canvas.selectedIndex)
        }
    }

    Shortcut {
        sequence: "Home"
        onActivated: canvas.fitToNodes()
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
        z: -2
        renderTarget: Canvas.FramebufferObject
        onPaint: {
            var context = getContext("2d")
            context.reset()
            context.fillStyle = Theme.canvasBg
            context.fillRect(0, 0, width, height)
            var spacing = 24 * canvas.zoomFactor
            var startX = ((canvas.panOffset.x % spacing) + spacing) % spacing
            var startY = ((canvas.panOffset.y % spacing) + spacing) % spacing
            context.strokeStyle = Theme.gridLine
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

    GraphConnectionsItem {
        id: pipes
        anchors.fill: parent
        z: -1
        connections: graphController.connections
        preview: graphController.connectionPreview
        previewActive: graphController.connectionPending
        panOffset: canvas.panOffset
        zoomFactor: canvas.zoomFactor
        layoutMode: canvas.pipeLayout
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
                required property string nodeIconSource
                required property string nodeAccent
                required property bool nodeEnabled
                required property string nodeLabel
                required property real nodeWidth
                required property real nodeHeight
                required property string nodeColor
                required property bool nodeIsBackdrop
                required property bool nodeIsGroup
                required property int nodeGroupId
                required property bool nodeSelected
                required property string nodePreviewSource
                required property int nodePreviewWidth
                required property int nodePreviewHeight
                required property int nodePreviewChannels
                required property var nodeInputPorts
                required property var nodeOutputPorts
                required property real nodeX
                required property real nodeY
                required property int inputPortCount
                required property int outputPortCount
                required property var nodeOnNodeProperties
                required property string nodeContentUrl
                required property bool nodeFixed

                x: nodeItem.nodeX
                y: nodeItem.nodeY
                width: nodeItem.nodeWidth
                height: nodeItem.nodeHeight
                z: nodeItem.nodeIsBackdrop
                   ? -0.5
                   : (nodeItem.nodeIsGroup
                      ? -0.25
                      : (nodeItem.nodeGroupId >= 0 ? 0.1 : 0))
                color: nodeItem.nodeIsBackdrop
                       ? nodeItem.nodeColor
                       : (nodeItem.nodeSelected ? Theme.nodeBgSelected
                                                : Theme.nodeBg)
                border.color: nodeItem.nodeSelected
                              ? Theme.nodeBorderSelected
                              : (nodeItem.nodeIsBackdrop ? nodeItem.nodeAccent
                                                         : Theme.nodeBorder)
                border.width: nodeItem.nodeSelected ? 2 : Theme.borderWidth
                radius: Theme.nodeRadius
                opacity: nodeItem.nodeEnabled ? 1.0 : 0.55

                Rectangle {
                    width: parent.width
                    height: 28
                    color: nodeItem.nodeAccent
                    radius: Theme.nodeRadius

                    Rectangle {
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        width: 28
                        height: 20
                        radius: Theme.radiusSmall
                        color: "#26000000"

                        Label {
                            anchors.centerIn: parent
                            text: nodeItem.nodeIcon
                            color: "#f1f6fa"
                            font.pixelSize: 9
                            font.bold: true
                        }

                        Image {
                            anchors.centerIn: parent
                            width: 16
                            height: 16
                            source: nodeItem.nodeIconSource
                            fillMode: Image.PreserveAspectFit
                            opacity: 0.9
                        }
                    }

                    Label {
                        anchors.left: parent.left
                        anchors.leftMargin: 40
                        anchors.right: parent.right
                        anchors.rightMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        text: nodeItem.nodeLabel
                        color: "#f1f6fa"
                        font.bold: true
                        elide: Text.ElideRight
                    }
                }

                Column {
                    x: 8
                    y: 32
                    width: parent.width - 16
                    spacing: 1
                    visible: nodeItem.nodeContentUrl.length === 0 &&
                             nodeItem.nodePreviewSource.length === 0
                    Repeater {
                        model: nodeItem.nodeOnNodeProperties
                        delegate: Label {
                            required property var modelData
                            text: modelData.name + ": " + String(modelData.value)
                            color: Theme.nodeBodyText
                            font.pixelSize: 11
                            elide: Text.ElideRight
                            width: parent.width
                        }
                    }
                }

                Image {
                    x: 8
                    y: 34
                    width: parent.width - 16
                    height: 50
                    visible: nodeItem.nodeContentUrl.length === 0 &&
                             nodeItem.nodePreviewSource.length > 0
                    source: nodeItem.nodePreviewSource
                    fillMode: Image.PreserveAspectFit
                    asynchronous: true
                    cache: false
                }

                Loader {
                    id: contentLoader
                    x: 6
                    y: 32
                    width: parent.width - 12
                    height: parent.height - 38
                    visible: nodeItem.nodeContentUrl.length > 0
                    source: nodeItem.nodeContentUrl
                    asynchronous: true
                    onLoaded: {
                        if (item) {
                            item.nodeRow = nodeItem.index
                            item.nodeController = canvas.controller
                        }
                    }
                }

                Label {
                    visible: nodeItem.nodeContentUrl.length === 0 &&
                             nodeItem.nodePreviewSource.length === 0 &&
                             (nodeItem.nodeIsBackdrop || nodeItem.nodeIsGroup)
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 6
                    text: nodeItem.nodeIsBackdrop
                          ? qsTr("Backdrop")
                          : qsTr("Group  %1 -> %2").arg(
                                nodeItem.inputPortCount).arg(
                                nodeItem.outputPortCount)
                    color: Theme.nodeMutedText
                }

                Label {
                    visible: nodeItem.nodeContentUrl.length === 0 &&
                             nodeItem.nodePreviewSource.length > 0
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 6
                    text: qsTr("%1 x %2  |  %3 ch")
                          .arg(nodeItem.nodePreviewWidth)
                          .arg(nodeItem.nodePreviewHeight)
                          .arg(nodeItem.nodePreviewChannels)
                    color: Theme.nodeMutedText
                }

                Row {
                    visible: nodeItem.nodeContentUrl.length === 0 &&
                             nodeItem.nodePreviewSource.length === 0 &&
                             !nodeItem.nodeIsBackdrop && !nodeItem.nodeIsGroup
                    anchors.centerIn: parent
                    spacing: 14

                    Column {
                        spacing: 2
                        Repeater {
                            model: nodeItem.nodeInputPorts
                            delegate: Label {
                                required property var modelData
                                text: modelData.name
                                color: Theme.nodeBodyText
                                font.pixelSize: 12
                            }
                        }
                    }

                    Label {
                        text: qsTr("->")
                        color: Theme.nodeMutedText
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    Column {
                        spacing: 2
                        Repeater {
                            model: nodeItem.nodeOutputPorts
                            delegate: Label {
                                required property var modelData
                                text: modelData.name
                                color: Theme.nodeBodyText
                                font.pixelSize: 12
                            }
                        }
                    }
                }

                Rectangle {
                    x: -6
                    y: nodeItem.height / 2 - 6
                    width: 12
                    height: 12
                    radius: 6
                    color: Theme.portInput
                    border.color: Theme.panelBg
                    border.width: 2
                    z: 2
                }

                Rectangle {
                    x: parent.width - 6
                    y: nodeItem.height / 2 - 6
                    width: 12
                    height: 12
                    radius: 6
                    color: canvas.controller.connectionPending &&
                           canvas.controller.pendingOutputRow === nodeItem.index
                           ? Theme.portActive
                           : Theme.portOutput
                    border.color: Theme.panelBg
                    border.width: 2
                    z: 2

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton
                        preventStealing: true

                        onPressed: function(mouse) {
                            mouse.accepted = true
                            canvas.controller.beginConnection(nodeItem.index)
                        }
                        onPositionChanged: function(mouse) {
                            var point = parent.mapToItem(canvas, mouse.x, mouse.y)
                            canvas.controller.updateConnectionPreview(
                                (point.x - canvas.panOffset.x) /
                                canvas.zoomFactor,
                                (point.y - canvas.panOffset.y) /
                                canvas.zoomFactor)
                        }
                        onReleased: function(mouse) {
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

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function(mouse) {
                        canvas.forceActiveFocus()
                        if (mouse.button === Qt.RightButton) {
                            canvas.contextNodeRow = nodeItem.index
                            canvas.selectedIndex = nodeItem.index
                            canvas.controller.selectNode(nodeItem.index)
                            nodeContextMenu.x = mouse.x
                            nodeContextMenu.y = mouse.y
                            nodeContextMenu.open()
                            mouse.accepted = true
                            return
                        }
                        if ((mouse.modifiers & (Qt.ControlModifier |
                                                Qt.MetaModifier |
                                                Qt.ShiftModifier)) !== 0) {
                            canvas.controller.toggleNodeSelection(nodeItem.index)
                        } else {
                            canvas.controller.selectNode(nodeItem.index)
                        }
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
                            canvas.controller.moveSelectedNodes(
                                nodeItem.index,
                                startPosition.x + translation.x / canvas.zoomFactor,
                                startPosition.y + translation.y / canvas.zoomFactor)
                    }
                }

                Rectangle {
                    id: resizeHandle
                    visible: !nodeItem.nodeIsBackdrop && !nodeItem.nodeIsGroup
                    x: parent.width - 9
                    y: parent.height - 9
                    width: 14
                    height: 14
                    radius: 3
                    color: Theme.nodeMutedText
                    opacity: resizeArea.pressed ? 1.0 : 0.55
                    z: 4

                    MouseArea {
                        id: resizeArea
                        anchors.fill: parent
                        cursorShape: Qt.SizeFDiagCursor
                        preventStealing: true

                        onPressed: function(mouse) {
                            mouse.accepted = true
                        }
                        onPositionChanged: function(mouse) {
                            var point = parent.mapToItem(canvas, mouse.x, mouse.y)
                            var worldX = (point.x - canvas.panOffset.x) /
                                         canvas.zoomFactor
                            var worldY = (point.y - canvas.panOffset.y) /
                                         canvas.zoomFactor
                            canvas.controller.resizeNode(
                                nodeItem.index,
                                Math.max(120, worldX - nodeItem.nodeX),
                                Math.max(72, worldY - nodeItem.nodeY))
                        }
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
        color: Qt.alpha(Theme.nodeBorderSelected, 0.18)
        border.color: Theme.nodeBorderSelected
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
            context.strokeStyle = Theme.portActive
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
            canvas.forceActiveFocus()
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
            canvas.selectionAdditive =
                    (mouse.modifiers & (Qt.ControlModifier |
                                        Qt.MetaModifier |
                                        Qt.ShiftModifier)) !== 0
            canvas.selectionStart = Qt.point(mouse.x, mouse.y)
            canvas.selectionEnd = canvas.selectionStart
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
            if (canvas.selecting) {
                canvas.controller.selectNodesInRect(
                    (canvas.selectionStart.x - canvas.panOffset.x) /
                    canvas.zoomFactor,
                    (canvas.selectionStart.y - canvas.panOffset.y) /
                    canvas.zoomFactor,
                    (canvas.selectionEnd.x - canvas.panOffset.x) /
                    canvas.zoomFactor,
                    (canvas.selectionEnd.y - canvas.panOffset.y) /
                    canvas.zoomFactor,
                    canvas.selectionAdditive)
                canvas.selecting = false
            }
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
