import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: tabSearch

    required property var controller
    property alias searchText: searchField.text

    width: 420
    height: 360
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    NodePaletteModel {
        id: searchModel
    }

    function acceptCurrent() {
        if (resultList.currentItem)
            resultList.currentItem.activate()
    }

    onOpened: {
        searchField.forceActiveFocus()
        resultList.currentIndex = 0
    }

    background: Rectangle {
        color: Theme.panelBg
        border.color: Theme.nodeBorderSelected
        border.width: 1
        radius: Theme.radius
    }

    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            text: qsTr("Tab Search")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: 16
        }

        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Type a node name")
            selectByMouse: true
            onTextChanged: {
                searchModel.filter = text
                resultList.currentIndex = 0
            }
            onAccepted: tabSearch.acceptCurrent()
        }

        ListView {
            id: resultList
            property var nodeController: tabSearch.controller
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 3
            model: searchModel
            focus: true
            highlightMoveDuration: 0

            delegate: Rectangle {
                required property int index
                required property string nodeType
                required property string nodeName
                required property string category
                width: resultList.width
                height: 40
                color: resultList.currentIndex === index ? Theme.nodeBgSelected
                                                         : Theme.nodeBg
                radius: Theme.radiusSmall

                function activate() {
                    resultList.nodeController.addNodeType(nodeType)
                    tabSearch.close()
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10

                    Label {
                        text: nodeName
                        color: Theme.textPrimary
                        Layout.fillWidth: true
                    }

                    Label {
                        text: category
                        color: Theme.textSecondary
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: parent.activate()
                }
            }

            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Up && currentIndex > 0) {
                    currentIndex -= 1
                    event.accepted = true
                } else if (event.key === Qt.Key_Down &&
                           currentIndex + 1 < count) {
                    currentIndex += 1
                    event.accepted = true
                } else if (event.key === Qt.Key_Return ||
                           event.key === Qt.Key_Enter) {
                    tabSearch.acceptCurrent()
                    event.accepted = true
                }
            }
        }
    }
}
