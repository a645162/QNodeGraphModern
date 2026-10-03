import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: propertiesBin

    required property var controller

    color: "#20262d"
    border.color: "#3c4652"
    radius: 4

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            text: qsTr("Properties")
            color: "#f0f3f6"
            font.bold: true
            font.pixelSize: 16
        }

        Label {
            Layout.fillWidth: true
            text: propertiesBin.controller.selectedRow >= 0
                  ? qsTr("Node %1").arg(propertiesBin.controller.selectedRow + 1)
                  : qsTr("Select a node")
            color: "#9aa6b2"
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3c4652"
        }

        ListView {
            id: propertyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 8
            model: propertiesBin.controller.selectedProperties

            delegate: ColumnLayout {
                required property var modelData
                width: propertyList.width
                spacing: 4

                Label {
                    Layout.fillWidth: true
                    text: modelData.name
                    color: "#c7d1db"
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true

                    CheckBox {
                        visible: modelData.valueType === "bool"
                        checked: Boolean(modelData.value)
                        onToggled: propertiesBin.controller.setNodeProperty(
                            propertiesBin.controller.selectedRow,
                            modelData.name, checked)
                    }

                    TextField {
                        visible: modelData.valueType !== "bool"
                        Layout.fillWidth: true
                        text: String(modelData.value)
                        selectByMouse: true
                        onEditingFinished: propertiesBin.controller.setNodeProperty(
                            propertiesBin.controller.selectedRow,
                            modelData.name, text)
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: propertyList.count === 0
                text: qsTr("No editable properties")
                color: "#738292"
            }
        }
    }
}
