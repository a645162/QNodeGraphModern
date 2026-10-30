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
    property var helpProvider: null
    readonly property bool darkTheme: Theme.dark
    function setThemeMode(value) {
        Theme.mode = value
        grid.requestPaint()
    }
    property real zoomFactor: 1.0
    property int pipeLayout: 0
    // Host applications may override the wire color without changing the
    // shared theme singleton used by other graph canvases.
    property color wireColor: Theme.wire
    property point panOffset: Qt.point(0, 0)
    // 平移状态机：中键 / 右键 / Alt+左键三种手势共用。右键在「按下但没拖动」
    // 时仍要弹上下文菜单，因此不能一按下就当平移，必须等位移超过阈值；
    // panConsumedClick 用来告诉各 MouseArea 的 onClicked「这次点击已被
    // 平移吞掉」。
    property bool panning: false
    property bool panMoved: false
    property bool panConsumedClick: false
    property point panPressPoint: Qt.point(0, 0)
    property point panStartOffset: Qt.point(0, 0)
    property int selectedIndex: -1
    property bool selecting: false
    property point selectionStart: Qt.point(0, 0)
    property point selectionEnd: Qt.point(0, 0)
    property bool selectionAdditive: false
    property bool slicing: false
    property point sliceStart: Qt.point(0, 0)
    property point sliceEnd: Qt.point(0, 0)
    property int contextNodeRow: -1
    // 批量删除确认：待删行号与因固定而被跳过的数量（提示里需要说明）。
    property var pendingDeleteRows: []
    property int pendingDeleteFixedCount: 0
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

    // 中键、右键、Alt+左键都用于平移视图（与 NodeGraphQt 一致）。
    function wantsPan(mouse) {
        if (mouse.button === Qt.MiddleButton ||
                mouse.button === Qt.RightButton)
            return true
        return mouse.button === Qt.LeftButton &&
                (mouse.modifiers & Qt.AltModifier) !== 0
    }

    // suppressClick：Alt+左键从头到尾就只用于平移，即使没有移动也不应该
    // 触发节点选择；其余手势只有真的拖动过才吞掉随后的 click。
    function beginPan(canvasPoint, suppressClick) {
        panning = true
        panMoved = false
        panConsumedClick = suppressClick === true
        panPressPoint = canvasPoint
        panStartOffset = panOffset
    }

    function updatePan(canvasPoint) {
        if (!panning)
            return
        var dx = canvasPoint.x - panPressPoint.x
        var dy = canvasPoint.y - panPressPoint.y
        // 2px 阈值：手抖不该被当成拖动，否则右键菜单会时灵时不灵。
        if (!panMoved && (Math.abs(dx) > 2 || Math.abs(dy) > 2))
            panMoved = true
        if (panMoved)
            panOffset = Qt.point(panStartOffset.x + dx,
                                 panStartOffset.y + dy)
    }

    // 返回「这次手势是否真的平移过」，调用方据此决定右键是否弹菜单。
    function endPan() {
        if (panning && panMoved)
            panConsumedClick = true
        panning = false
        panMoved = false
        return panConsumedClick
    }

    // 固定节点（sink / 源等）不允许删除，框选时常常一起被框进来，所以先剔除。
    function deletableRowsFrom(rows) {
        var result = []
        for (var i = 0; i < rows.length; ++i) {
            var row = rows[i]
            if (row < 0 || row >= graphController.nodeCount)
                continue
            if (graphController.index(row, 0).data(
                        GraphController.NodeFixedRole))
                continue
            result.push(row)
        }
        return result
    }

    // Del / Backspace 的统一入口：连线优先，其次多选，最后退回单选。
    // 删除多个节点前必须确认，因为框选很容易多框进一个节点。
    function requestDeleteSelection() {
        if (deleteConfirmDialog.opened)
            return
        if (canvas.controller.removeSelectedConnection()) {
            canvas.forceActiveFocus()
            return
        }
        var rows = graphController.selectedRows
        if (!rows || rows.length === 0)
            rows = canvas.selectedIndex >= 0 ? [canvas.selectedIndex] : []
        if (rows.length === 0)
            return
        var deletable = canvas.deletableRowsFrom(rows)
        if (deletable.length === 0)
            return
        canvas.pendingDeleteRows = deletable
        canvas.pendingDeleteFixedCount = rows.length - deletable.length
        if (deletable.length === 1 && canvas.pendingDeleteFixedCount === 0) {
            canvas.controller.deleteNode(deletable[0])
            canvas.pendingDeleteRows = []
            return
        }
        deleteConfirmDialog.open()
    }

    function confirmDeleteSelection() {
        var rows = canvas.pendingDeleteRows
        canvas.pendingDeleteRows = []
        canvas.pendingDeleteFixedCount = 0
        if (rows.length > 0)
            canvas.controller.deleteNodes(rows)
        canvas.forceActiveFocus()
    }

    GraphController {
        id: graphController
        objectName: "workflowGraphController"
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
            text: qsTr("添加灰度节点")
            onTriggered: canvas.controller.addNodeType("grayscale")
        }

        MenuItem {
            text: qsTr("自动布局")
            onTriggered: canvas.controller.autoLayout()
        }

        MenuItem {
            text: qsTr("清空图表")
            onTriggered: canvas.controller.clearGraph()
        }

        MenuSeparator {}

        MenuItem {
            text: qsTr("节点帮助")
            enabled: canvas.contextNodeRow >= 0 && canvas.helpProvider !== null
            onTriggered: {
                if (canvas.helpProvider && canvas.contextNodeRow >= 0) {
                    var typeId = graphController.data(
                                graphController.index(canvas.contextNodeRow, 0),
                                GraphController.NodeTypeRole)
                    canvas.helpProvider.openFor(String(typeId))
                }
            }
        }

        MenuItem {
            text: qsTr("重置视图")
            onTriggered: canvas.resetView()
        }
    }

    Menu {
        id: nodeContextMenu

        Menu {
            title: qsTr("添加到组")

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
            text: qsTr("删除节点")
            enabled: canvas.contextNodeRow >= 0 &&
                     !(graphController.index(canvas.contextNodeRow, 0)
                       .data(GraphController.NodeFixedRole))
            onTriggered: {
                if (canvas.contextNodeRow >= 0)
                    canvas.controller.deleteNode(canvas.contextNodeRow)
            }
        }

        MenuItem {
            text: qsTr("从组中移除")
            enabled: canvas.contextNodeRow >= 0
            onTriggered: {
                if (canvas.contextNodeRow >= 0)
                    graphController.clearNodeGroup(canvas.contextNodeRow)
            }
        }

        MenuItem {
            text: qsTr("清除选择")
            onTriggered: {
                canvas.contextNodeRow = -1
                canvas.selectedIndex = -1
                canvas.controller.selectNode(-1)
            }
        }
    }

    // 框选后一次删多个节点属于破坏性操作，先确认再删。
    Dialog {
        id: deleteConfirmDialog
        anchors.centerIn: parent
        title: qsTr("删除节点")
        modal: true
        focus: true
        standardButtons: Dialog.Ok | Dialog.Cancel

        Label {
            // 固定宽度：用 implicitWidth 反推宽度会让 Dialog 的 implicitWidth
            // 与文本换行宽度互相依赖，触发运行期 “Binding loop detected for
            // property implicitWidth” 警告。
            width: 320
            wrapMode: Text.WordWrap
            text: {
                var message = qsTr("删除选中的 %1 个节点？").arg(
                              canvas.pendingDeleteRows.length)
                if (canvas.pendingDeleteFixedCount > 0)
                    message += "\n" + qsTr("将保留 %1 个固定节点。").arg(
                               canvas.pendingDeleteFixedCount)
                return message
            }
        }

        onAccepted: canvas.confirmDeleteSelection()
        onRejected: {
            canvas.pendingDeleteRows = []
            canvas.pendingDeleteFixedCount = 0
            canvas.forceActiveFocus()
        }
    }

    Shortcut {
        sequence: "Delete"
        onActivated: canvas.requestDeleteSelection()
    }

    Shortcut {
        sequence: "Backspace"
        onActivated: canvas.requestDeleteSelection()
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
        z: 0
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
        z: 1
        connections: graphController.connections
        preview: graphController.connectionPreview
        previewActive: graphController.connectionPending
        panOffset: canvas.panOffset
        zoomFactor: canvas.zoomFactor
        layoutMode: canvas.pipeLayout
        wireColor: canvas.wireColor
    }

    Item {
        id: world
        x: canvas.panOffset.x
        y: canvas.panOffset.y
        scale: canvas.zoomFactor
        z: 2

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
                required property bool nodeOutputUsed
                required property bool nodeEffective
                required property bool nodeInputConnected
                required property bool nodeOutputConnected

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
                // 输出未被使用的节点（剪枝）额外降级，与「已禁用」区分：
                // 禁用是用户主动关闭，剪枝是接线问题。
                opacity: nodeItem.nodeEffective
                         ? (nodeItem.nodeEnabled ? 1.0 : 0.55)
                         : 0.38

                // 剪枝标记：虚线红框 + 角标。Theme.error 此前零引用，正好作
                // 无效标识色。
                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 1
                    color: "transparent"
                    radius: Theme.nodeRadius
                    border.color: Theme.error
                    border.width: 1
                    visible: !nodeItem.nodeEffective
                    z: 5
                }

                Rectangle {
                    id: nodeHeader
                    width: parent.width
                    height: 28
                    z: 1
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

                    DragHandler {
                        acceptedButtons: Qt.LeftButton
                        target: null
                        property point startPosition: Qt.point(0, 0)
                        // Alt+左键是平移手势，此时不能顺手把节点也拖走。
                        onActiveChanged: {
                            if (active && !canvas.panning) {
                                if (canvas.controller.selectedRow !==
                                    nodeItem.index)
                                    canvas.controller.selectNode(nodeItem.index)
                                startPosition = Qt.point(nodeItem.nodeX,
                                                         nodeItem.nodeY)
                            }
                        }
                        onTranslationChanged: {
                            if (active && !canvas.panning)
                                canvas.controller.moveSelectedNodes(
                                    nodeItem.index,
                                    startPosition.x + translation.x /
                                        canvas.zoomFactor,
                                    startPosition.y + translation.y /
                                        canvas.zoomFactor)
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton |
                                         Qt.MiddleButton
                        onPressed: function(mouse) {
                            canvas.forceActiveFocus()
                            if (!canvas.wantsPan(mouse))
                                return
                            canvas.beginPan(nodeItem.mapToItem(canvas,
                                                               mouse.x,
                                                               mouse.y),
                                            mouse.button === Qt.LeftButton)
                            mouse.accepted = true
                        }
                        onPositionChanged: function(mouse) {
                            if (canvas.panning)
                                canvas.updatePan(nodeItem.mapToItem(
                                                     canvas, mouse.x, mouse.y))
                        }
                        onReleased: function(mouse) {
                            if (!canvas.panning)
                                return
                            var moved = canvas.endPan()
                            // 右键只是按下没拖动 → 仍然弹节点菜单。
                            if (mouse.button === Qt.RightButton && !moved) {
                                canvas.contextNodeRow = nodeItem.index
                                canvas.selectedIndex = nodeItem.index
                                canvas.controller.selectNode(nodeItem.index)
                                var menuPoint = nodeItem.mapToItem(
                                    canvas, mouse.x, mouse.y)
                                nodeContextMenu.popup(menuPoint.x,
                                                       menuPoint.y)
                            }
                        }
                        onClicked: function(mouse) {
                            // 平移吞掉的点击不能再当成选择。
                            if (canvas.panConsumedClick ||
                                mouse.button !== Qt.LeftButton)
                                return
                            canvas.forceActiveFocus()
                            if ((mouse.modifiers & (Qt.ControlModifier |
                                                    Qt.MetaModifier |
                                                    Qt.ShiftModifier)) !== 0)
                                canvas.controller.toggleNodeSelection(
                                    nodeItem.index)
                            else
                                canvas.controller.selectNode(nodeItem.index)
                        }
                    }
                }

                Column {
                    x: 8
                    y: 32
                    width: parent.width - 16
                    spacing: 3
                    visible: nodeItem.nodeContentUrl.length === 0 &&
                             nodeItem.nodePreviewSource.length === 0
                    Repeater {
                        model: nodeItem.nodeOnNodeProperties

                        // 节点内属性控件：按描述符声明的 editor 分派，而不是
                        // 一律当只读文本。此前这里是 Label，滑块做不出来。
                        delegate: Row {
                            id: propertyRow
                            required property var modelData
                            // 删除节点时 Repeater 会先解绑 delegate 再销毁，
                            // 此刻 parent 已为 null，直接取 parent.width 会
                            // 抛 TypeError 警告，因此加空值保护。
                            width: parent ? parent.width : 0
                            property bool basicVisible:
                                String(modelData.visibility || "basic") ===
                                "basic"
                            visible: basicVisible
                            height: visible ? implicitHeight : 0
                            spacing: 4

                            HoverHandler { id: propertyHover }
                            ToolTip.visible: propertyHover.hovered &&
                                             String(modelData.description || "").length > 0
                            ToolTip.text: String(modelData.description || "")

                            Label {
                                text: propertyRow.modelData.name
                                color: Theme.nodeBodyText
                                font.pixelSize: 10
                                width: 52
                                elide: Text.ElideRight
                            }

                            // 数值滑块（editor == "slider"）
                            Slider {
                                z: 3
                                visible: propertyRow.modelData.editor === "slider"
                                width: Math.max(40, propertyRow.width - 144)
                                from: propertyRow.modelData.minimum
                                to: Math.max(propertyRow.modelData.minimum + 1,
                                             propertyRow.modelData.maximum)
                                stepSize: propertyRow.modelData.step > 0
                                          ? propertyRow.modelData.step : 0.001
                                value: Number(propertyRow.modelData.value)
                                // 拖动中即时回写，靠库的脏传播驱动下游重算。
                                onValueChanged: {
                                    if (pressed)
                                        canvas.controller.setNodeProperty(
                                                    nodeItem.index,
                                                    propertyRow.modelData.name,
                                                    value)
                                }
                            }

                            TextField {
                                z: 3
                                visible: propertyRow.modelData.editor === "slider"
                                width: 48
                                text: Number(propertyRow.modelData.value).toLocaleString(
                                          Qt.locale(), 'f',
                                          Number(propertyRow.modelData.step) < 1 ? 2 : 0)
                                font.pixelSize: 10
                                selectByMouse: true
                                inputMethodHints: Qt.ImhFormattedNumbersOnly
                                onEditingFinished: canvas.controller.setNodeProperty(
                                                       nodeItem.index,
                                                       propertyRow.modelData.name,
                                                       text)
                            }

                            // 布尔开关
                            CheckBox {
                                z: 3
                                visible: propertyRow.modelData.editor !== "slider" &&
                                         propertyRow.modelData.valueType === "bool" &&
                                         propertyRow.modelData.name !== "enabled"
                                text: propertyRow.modelData.name
                                scale: 0.8
                                checked: Boolean(propertyRow.modelData.value)
                                onToggled: canvas.controller.setNodeProperty(
                                               nodeItem.index,
                                               propertyRow.modelData.name,
                                               checked)
                            }

                            // 其余一律文本输入（字符串/未声明控件的数值）
                            TextField {
                                z: 3
                                visible: propertyRow.modelData.editor !== "slider" &&
                                         propertyRow.modelData.valueType !== "bool"
                                width: Math.max(40, propertyRow.width - 60)
                                text: String(propertyRow.modelData.value)
                                font.pixelSize: 10
                                onEditingFinished: canvas.controller.setNodeProperty(
                                                       nodeItem.index,
                                                       propertyRow.modelData.name,
                                                       text)
                            }
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
                    z: 3
                    source: nodeItem.nodeContentUrl
                    asynchronous: true
                    property string previewSource: nodeItem.nodePreviewSource
                    onLoaded: {
                        if (item) {
                            item.nodeRow = nodeItem.index
                            item.nodeController = canvas.controller
                            if (item.previewSource !== undefined)
                                item.previewSource = previewSource
                        }
                    }
                    onPreviewSourceChanged: {
                        if (item && item.previewSource !== undefined)
                            item.previewSource = previewSource
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
                          ? qsTr("背景板")
                          : qsTr("组  %1 -> %2").arg(
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
                    text: qsTr("%1 x %2  |  %3 通道")
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
                    visible: nodeItem.inputPortCount > 0
                    radius: 6
                    color: nodeItem.nodeInputConnected
                           ? Theme.portInput : Theme.error
                    border.color: nodeItem.nodeInputConnected
                                  ? Theme.panelBg : Theme.error
                    border.width: nodeItem.nodeInputConnected ? 2 : 3
                    z: 2

                    Label {
                        anchors.centerIn: parent
                        visible: !nodeItem.nodeInputConnected
                        text: "!"
                        color: Theme.panelBg
                        font.bold: true
                        font.pixelSize: 9
                    }
                }

                Rectangle {
                    x: parent.width - 6
                    y: nodeItem.height / 2 - 6
                    width: 12
                    height: 12
                    visible: nodeItem.outputPortCount > 0
                    radius: 6
                    color: canvas.controller.connectionPending &&
                           canvas.controller.pendingOutputRow === nodeItem.index
                           ? Theme.portActive
                           : (nodeItem.nodeOutputConnected
                              ? Theme.portOutput : Theme.error)
                    border.color: nodeItem.nodeOutputConnected
                                  ? Theme.panelBg : Theme.error
                    border.width: nodeItem.nodeOutputConnected ? 2 : 3
                    z: 2

                    Label {
                        anchors.centerIn: parent
                        visible: !nodeItem.nodeOutputConnected &&
                                 !canvas.controller.connectionPending
                        text: "!"
                        color: Theme.panelBg
                        font.bold: true
                        font.pixelSize: 9
                    }

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
                    anchors.topMargin: nodeHeader.height
                    acceptedButtons: Qt.LeftButton | Qt.RightButton |
                                     Qt.MiddleButton
                    onPressed: function(mouse) {
                        canvas.forceActiveFocus()
                        if (!canvas.wantsPan(mouse))
                            return
                        canvas.beginPan(nodeItem.mapToItem(canvas, mouse.x,
                                                           mouse.y),
                                        mouse.button === Qt.LeftButton)
                        mouse.accepted = true
                    }
                    onPositionChanged: function(mouse) {
                        if (canvas.panning)
                            canvas.updatePan(nodeItem.mapToItem(
                                                 canvas, mouse.x, mouse.y))
                    }
                    onReleased: function(mouse) {
                        if (!canvas.panning)
                            return
                        var moved = canvas.endPan()
                        if (mouse.button === Qt.RightButton && !moved) {
                            canvas.contextNodeRow = nodeItem.index
                            canvas.selectedIndex = nodeItem.index
                            canvas.controller.selectNode(nodeItem.index)
                            var menuPoint = nodeItem.mapToItem(canvas,
                                                               mouse.x,
                                                               mouse.y)
                            nodeContextMenu.popup(menuPoint.x, menuPoint.y)
                        }
                    }
                    onClicked: function(mouse) {
                        if (canvas.panConsumedClick ||
                                mouse.button !== Qt.LeftButton)
                            return
                        canvas.forceActiveFocus()
                        if ((mouse.modifiers & (Qt.ControlModifier |
                                                Qt.MetaModifier |
                                                Qt.ShiftModifier)) !== 0) {
                            canvas.controller.toggleNodeSelection(nodeItem.index)
                        } else {
                            canvas.controller.selectNode(nodeItem.index)
                        }
                    }
                }

                // 四角 resize。此前只有右下角一个手柄，且 resizeNode(row,w,h)
                // 的签名表达不了「同时改变左上角」—— 从左/上侧拖拽必须同时移动
                // origin，因此改用 resizeNodeAt(row, x, y, w, h)。
                //
                // 每个角记录拖拽起点的矩形（origin + size），全程按「起点 + 位移」
                // 计算，避免逐帧累加导致的漂移。
                Repeater {
                    model: [
                        { corner: "tl", cx: 0, cy: 0, cursor: Qt.SizeFDiagCursor },
                        { corner: "tr", cx: 1, cy: 0, cursor: Qt.SizeBDiagCursor },
                        { corner: "bl", cx: 0, cy: 1, cursor: Qt.SizeBDiagCursor },
                        { corner: "br", cx: 1, cy: 1, cursor: Qt.SizeFDiagCursor }
                    ]

                    delegate: Rectangle {
                        id: resizeHandle
                        required property var modelData

                        visible: nodeItem.nodeSelected &&
                                 !nodeItem.nodeIsBackdrop &&
                                 !nodeItem.nodeIsGroup
                        // 批量删除会整表 reset，delegate 销毁前 parent 可能已经
                        // 变 null；这里做空值保护，避免运行期 TypeError 警告。
                        x: modelData.cx === 0 ? -7
                                              : (parent ? parent.width - 7 : -7)
                        y: modelData.cy === 0 ? -7
                                              : (parent ? parent.height - 7 : -7)
                        width: 14
                        height: 14
                        radius: 3
                        color: Theme.nodeMutedText
                        opacity: resizeArea.pressed ? 1.0 : 0.55
                        z: 4

                        MouseArea {
                            id: resizeArea
                            anchors.fill: parent
                            cursorShape: resizeHandle.modelData.cursor
                            preventStealing: true

                            property real startX: 0
                            property real startY: 0
                            property real startWidth: 0
                            property real startHeight: 0

                            onPressed: function(mouse) {
                                startX = nodeItem.nodeX
                                startY = nodeItem.nodeY
                                startWidth = nodeItem.nodeWidth
                                startHeight = nodeItem.nodeHeight
                                mouse.accepted = true
                            }
                            onPositionChanged: function(mouse) {
                                var point = parent.mapToItem(canvas, mouse.x,
                                                             mouse.y)
                                var worldX = (point.x - canvas.panOffset.x) /
                                             canvas.zoomFactor
                                var worldY = (point.y - canvas.panOffset.y) /
                                             canvas.zoomFactor
                                var left = startX
                                var top = startY
                                var right = startX + startWidth
                                var bottom = startY + startHeight
                                // 只改动被拖拽的那条边，对边保持不动。
                                if (resizeHandle.modelData.cx === 0)
                                    left = Math.min(worldX, right - 120)
                                else
                                    right = Math.max(worldX, left + 120)
                                if (resizeHandle.modelData.cy === 0)
                                    top = Math.min(worldY, bottom - 72)
                                else
                                    bottom = Math.max(worldY, top + 72)
                                canvas.controller.resizeNodeAt(
                                    nodeItem.index, left, top,
                                    right - left, bottom - top)
                            }
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
        z: 0
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        onPressed: function(mouse) {
            canvas.forceActiveFocus()
            // 中键 / 右键 / Alt+左键在这里（以及节点上的 MouseArea）平移视图；
            // 右键菜单改到 release 且未拖动时才弹出。
            if (canvas.wantsPan(mouse)) {
                canvas.beginPan(Qt.point(mouse.x, mouse.y),
                                mouse.button === Qt.LeftButton)
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
            var worldX = (mouse.x - canvas.panOffset.x) / canvas.zoomFactor
            var worldY = (mouse.y - canvas.panOffset.y) / canvas.zoomFactor
            if (canvas.controller.selectConnectionAt(worldX, worldY)) {
                canvas.controller.selectNode(-1)
                canvas.selectedIndex = -1
                canvas.selecting = false
                mouse.accepted = true
                return
            }
            if ((mouse.modifiers & (Qt.ControlModifier |
                                    Qt.MetaModifier |
                                    Qt.ShiftModifier)) === 0) {
                canvas.controller.selectNode(-1)
                canvas.selectedIndex = -1
                canvas.contextNodeRow = -1
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
            if (canvas.panning) {
                canvas.updatePan(Qt.point(mouse.x, mouse.y))
                return
            }
            if (canvas.slicing) {
                canvas.sliceEnd = Qt.point(mouse.x, mouse.y)
                sliceGuide.requestPaint()
                return
            }
            if (canvas.selecting)
                canvas.selectionEnd = Qt.point(mouse.x, mouse.y)
        }
        onReleased: function(mouse) {
            if (canvas.panning) {
                var moved = canvas.endPan()
                if (mouse.button === Qt.RightButton && !moved) {
                    canvas.contextNodeRow = -1
                    contextMenu.x = mouse.x
                    contextMenu.y = mouse.y
                    contextMenu.open()
                }
                mouse.accepted = true
                return
            }
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
}
