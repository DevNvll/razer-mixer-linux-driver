import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RazerMixer

ColumnLayout {
    id: root
    required property var settings
    spacing: 16
    RowLayout {
        Layout.fillWidth: true; spacing: 16
        ColorField {
            Layout.fillWidth: true; Layout.fillHeight: true
            label: "Active color"; settingKey: "color"
            description: "Device lighting and the app accent."
            colorValue: root.settings.color || "00ff40"
        }
        ColorField {
            Layout.fillWidth: true; Layout.fillHeight: true
            label: "Muted color"; settingKey: "muted-color"
            description: "Button color when a channel is muted."
            colorValue: root.settings.muted_color || "ff0000"
        }
    }
    Rectangle {
        Layout.fillWidth: true; implicitHeight: 112
        color: Theme.panel; border.color: "#303030"
        ColumnLayout {
            anchors.fill: parent; anchors.margins: 16; spacing: 12
            RowLayout {
                Layout.fillWidth: true
                Text { text: "Brightness"; color: Theme.text; font.pixelSize: 15; font.weight: Font.DemiBold }
                Item { Layout.fillWidth: true }
                Text { text: brightness.value === 0 ? "Off" : Math.round(brightness.value) + "%"; color: Theme.accent; font.pixelSize: 15 }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 14
                Text { text: "0"; color: Theme.dim; font.pixelSize: 11 }
                Slider {
                    id: brightness; objectName: "brightness"
                    Layout.fillWidth: true; from: 0; to: 100; stepSize: 1
                    Accessible.name: "Lighting brightness"
                    onMoved: if (!pressed) Mixer.setLighting("brightness", String(Math.round(value)))
                    onPressedChanged: if (!pressed) Mixer.setLighting("brightness", String(Math.round(value)))
                    background: Rectangle {
                        x: brightness.leftPadding; y: brightness.topPadding + brightness.availableHeight / 2 - 2
                        width: brightness.availableWidth; height: 4; color: "#303030"
                        Rectangle { width: brightness.visualPosition * parent.width; height: parent.height; color: Theme.accent }
                    }
                    handle: Rectangle {
                        x: brightness.leftPadding + brightness.visualPosition * (brightness.availableWidth - width)
                        y: brightness.topPadding + brightness.availableHeight / 2 - height / 2
                        width: 12; height: 20; color: Theme.accent
                        border.color: brightness.activeFocus ? Theme.text : Theme.accent
                    }
                    Binding { target: brightness; property: "value"; value: root.settings.brightness === undefined ? 50 : root.settings.brightness; when: !brightness.pressed }
                }
                Text { text: "100"; color: Theme.dim; font.pixelSize: 11 }
            }
        }
    }
    Item { Layout.fillHeight: true }
    Text { Layout.fillWidth: true; text: "Changes save automatically and return when you reconnect the mixer."; color: Theme.dim; font.pixelSize: 12; wrapMode: Text.WordWrap }
}
