import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import RazerMixer

Rectangle {
    id: root
    required property string label
    required property string settingKey
    required property string colorValue
    required property string description
    color: Theme.panel
    border.color: "#303030"
    implicitHeight: content.implicitHeight + 32
    function openPicker() {
        picker.selectedColor = "#" + root.colorValue
        picker.open()
    }
    onColorValueChanged: if (!field.activeFocus) field.text = "#" + colorValue
    ColorDialog {
        id: picker
        objectName: root.settingKey + "Picker"
        title: "Choose " + root.label.toLowerCase()
        options: ColorDialog.DontUseNativeDialog
        onAccepted: Mixer.setLighting(root.settingKey, selectedColor.toString())
    }
    ColumnLayout {
        id: content
        anchors.fill: parent; anchors.margins: 16; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            Text { text: root.label; color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold }
            Item { Layout.fillWidth: true }
            Rectangle { width: 8; height: 8; color: "#" + root.colorValue }
        }
        Text { Layout.fillWidth: true; text: root.description; color: Theme.dim; font.pixelSize: 11; wrapMode: Text.WordWrap }
        Button {
            id: choose
            objectName: root.settingKey + "Choose"
            Layout.fillWidth: true; Layout.preferredHeight: 72
            Accessible.name: "Choose " + root.label.toLowerCase()
            background: Rectangle {
                color: "#" + root.colorValue
                border.color: choose.activeFocus ? Theme.text : "#505050"
                border.width: choose.activeFocus ? 2 : 1
            }
            contentItem: Column {
                anchors.centerIn: parent; spacing: 5
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: "Choose color"; color: Theme.foreground(choose.background.color); font.pixelSize: 14; font.weight: Font.DemiBold }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: "#" + root.colorValue.toUpperCase(); color: Theme.foreground(choose.background.color); font.pixelSize: 11; opacity: 0.8 }
            }
            onClicked: root.openPicker()
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            TextField {
                id: field; objectName: root.settingKey + "Field"
                Layout.fillWidth: true; implicitHeight: 32
                text: "#" + root.colorValue; color: Theme.text; font.pixelSize: 12
                maximumLength: 7; leftPadding: 10; selectByMouse: true
                Accessible.name: root.label + " hex value"
                validator: RegularExpressionValidator { regularExpression: /^#?[0-9a-fA-F]{6}$/ }
                background: Rectangle { color: Theme.background; border.color: field.activeFocus ? Theme.accent : "#404040" }
                onAccepted: if (acceptableInput) Mixer.setLighting(root.settingKey, text)
            }
            ActionButton { text: "Apply"; enabled: field.acceptableInput; onClicked: Mixer.setLighting(root.settingKey, field.text) }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 8
            Repeater {
                model: ["00ff40", "ff0000", "69c9ff", "cba6f7", "ffca70", "ffffff"]
                Button {
                    id: preset
                    required property string modelData
                    Layout.fillWidth: true; implicitHeight: 24
                    background: Rectangle {
                        color: "#" + preset.modelData
                        border.color: preset.activeFocus || root.colorValue === preset.modelData ? Theme.text : "transparent"
                        border.width: 2
                    }
                    Accessible.name: "Color #" + modelData
                    onClicked: Mixer.setLighting(root.settingKey, modelData)
                }
            }
        }
    }
}
