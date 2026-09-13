import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RazerMixer

Rectangle {
    id: root
    required property int channelIndex
    required property var channel
    readonly property bool available: channel.available || false
    readonly property bool muted: channel.muted || false
    color: Theme.background; radius: Theme.cornerRadius
    border.color: muted ? Theme.danger : "transparent"
    Rectangle { visible: root.channelIndex > 0 && !root.muted; width: 1; height: parent.height; color: "#252525" }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 14; spacing: 8
        RowLayout {
            Layout.fillWidth: true
            Text { text: "0" + (root.channelIndex + 1); color: Theme.dim; font.pixelSize: 11; font.letterSpacing: 2 }
            Item { Layout.fillWidth: true }
            Rectangle { width: 6; height: 6; radius: 3; color: root.muted ? Theme.danger : Theme.accent; opacity: root.available ? 1 : 0.2 }
        }
        Text { text: ["Master", "Chat", "Music", "Mic"][root.channelIndex]; color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold }
        RowLayout {
            spacing: 3
            Text { text: root.available ? Math.round(volume.value * 100) : "--"; color: root.muted ? Theme.danger : Theme.text; font.pixelSize: 32; font.weight: Font.DemiBold }
            Text { text: "%"; color: Theme.dim; font.pixelSize: 13; Layout.alignment: Qt.AlignBottom; Layout.bottomMargin: 9 }
        }
        Item {
            Layout.fillHeight: true; Layout.fillWidth: true; Layout.minimumHeight: 170
            Repeater {
                model: 5
                Rectangle {
                    required property int index
                    x: parent.width / 2 - 22; y: 12 + index * (parent.height - 24) / 4
                    width: 44; height: 1; color: Theme.border
                }
            }
            Item {
                id: volume
                objectName: "volume" + root.channelIndex
                anchors.centerIn: parent; width: 64; height: parent.height
                readonly property real value: root.channel.volume || 0
                readonly property real position: Math.max(0, Math.min(1, value))
                Accessible.role: Accessible.ProgressBar
                Accessible.name: root.channel.label + " volume"
                Accessible.description: Math.round(value * 100) + " percent. Adjust the physical fader on the mixer."
                Accessible.readOnly: true
                Rectangle {
                    x: volume.width / 2 - 3; y: 12
                    width: 6; height: volume.height - 24; radius: 3; color: Theme.background
                    Rectangle { anchors.bottom: parent.bottom; width: parent.width; height: volume.position * parent.height; radius: 3; color: root.muted ? Theme.danger : Theme.accent; opacity: root.available ? 1 : 0.2 }
                }
                Rectangle {
                    x: volume.width / 2 - width / 2
                    y: 5 + (1 - volume.position) * (volume.height - 24)
                    width: 30; height: 14
                    color: root.available ? Theme.text : Theme.border
                    border.color: Theme.background; border.width: 2
                    Rectangle { anchors.centerIn: parent; width: 16; height: 1; color: Theme.dim }
                }
            }
        }
        ActionButton {
            objectName: "mute" + root.channelIndex
            Layout.fillWidth: true; Layout.topMargin: 8
            enabled: root.available; checkable: true; checked: root.muted
            selectedColor: "#632c2c"
            text: checked ? "Unmute" : "Mute"
            onClicked: Mixer.setMute(root.channelIndex, checked)
        }
        Text {
            Layout.fillWidth: true
            text: ["Everything you hear", "Chat channel", "Music channel", "Microphone input"][root.channelIndex]
            color: Theme.dim; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; elide: Text.ElideRight
        }
    }
}
