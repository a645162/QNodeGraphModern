import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: propertiesBin

    required property var controller

    color: Theme.panelBg
    border.color: Theme.panelBorder
    radius: Theme.radius

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        Label {
            text: qsTr("Properties")
            color: Theme.textPrimary
            font.bold: true
            font.pixelSize: 16
        }

        Label {
            Layout.fillWidth: true
            text: propertiesBin.controller.selectedRow >= 0
                  ? qsTr("Node %1").arg(propertiesBin.controller.selectedRow + 1)
                  : qsTr("Select a node")
            color: Theme.textSecondary
            elide: Text.ElideRight
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.panelBorder
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
                    color: Theme.nodeBodyText
                    elide: Text.ElideRight
                }

                RowLayout {
                    Layout.fillWidth: true

                    CheckBox {
                        visible: modelData.editor === "checkbox" ||
                                 modelData.valueType === "bool"
                        checked: Boolean(modelData.value)
                        onToggled: propertiesBin.controller.setNodeProperty(
                            propertiesBin.controller.selectedRow,
                            modelData.name, checked)
                    }

                    Slider {
                        visible: modelData.editor === "slider"
                        Layout.fillWidth: true
                        from: Number(modelData.minimum)
                        to: Number(modelData.maximum)
                        stepSize: Number(modelData.step) > 0
                                  ? Number(modelData.step) : 1
                        value: Number(modelData.value)
                        onMoved: propertiesBin.controller.setNodeProperty(
                                     propertiesBin.controller.selectedRow,
                                     modelData.name, value)
                    }

                    ComboBox {
                        visible: modelData.editor === "combobox"
                        Layout.fillWidth: true
                        model: modelData.choices
                        currentIndex: Math.max(0, indexOfValue(
                            String(modelData.value)))
                        onActivated: propertiesBin.controller.setNodeProperty(
                                         propertiesBin.controller.selectedRow,
                                         modelData.name, currentText)
                    }

                    TextField {
                        visible: modelData.editor !== "checkbox" &&
                                 modelData.editor !== "slider" &&
                                 modelData.editor !== "combobox" &&
                                 modelData.valueType !== "bool"
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
                color: Theme.nodeMutedText
            }
        }
    }
}
