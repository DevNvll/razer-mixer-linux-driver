import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RazerMixer

ApplicationWindow {
    id: window
    width: 860; height: 640
    minimumWidth: 760; minimumHeight: 640
    visible: true
    title: "Razer Audio Mixer"
    color: Theme.background
    font.family: "monospace"
    font.pixelSize: 12
    palette.highlight: Theme.accent
    palette.highlightedText: Theme.foreground(Theme.accent)
    property int activePage: 0
    readonly property var device: Mixer.snapshot.device || ({})
    readonly property var settings: Mixer.snapshot.config || ({})
    Shortcut { sequence: "Ctrl+1"; onActivated: window.activePage = 0 }
    Shortcut { sequence: "Ctrl+2"; onActivated: window.activePage = 1 }
    Shortcut { sequence: "Ctrl+3"; onActivated: window.activePage = 2 }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 18
        RowLayout {
            spacing: 16
            Item {
                Layout.preferredWidth: 36; Layout.preferredHeight: 38
                Repeater {
                    model: 4
                    Rectangle {
                        required property int index
                        x: index * 9; y: 4; width: 3; height: 30; radius: 1.5; color: Theme.border
                        Rectangle { x: -2; y: [6,16,10,20][index]; width: 7; height: 5; radius: 1.5; color: Theme.accent }
                    }
                }
            }
            ColumnLayout {
                spacing: 5
                Text { text: "Razer Audio Mixer"; color: Theme.text; font.pixelSize: 20; font.weight: Font.DemiBold }
                Text { text: "Volume, routing and device lighting"; color: Theme.dim; font.pixelSize: 13 }
            }
            Item { Layout.fillWidth: true }
            Rectangle { width: 7; height: 7; radius: 4; color: window.device.connected ? Theme.accent : Theme.dim }
            Text { text: window.device.connected ? "Connected" : "Disconnected"; color: Theme.dim; font.pixelSize: 13 }
        }
        RowLayout {
            spacing: 8
            Repeater {
                model: ["Mixer", "Lighting", "Applications"]
                ActionButton {
                    required property string modelData
                    required property int index
                    objectName: "tab" + index
                    text: modelData; checked: window.activePage === index
                    onClicked: window.activePage = index
                }
            }
            Item { Layout.fillWidth: true }
            Text { text: "1532 : 053E"; color: Theme.dim; font.pixelSize: 11; font.letterSpacing: 2 }
        }
        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: window.activePage
            ColumnLayout {
                spacing: 16
                RowLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
                    Repeater {
                        model: 4
                        Channel {
                            required property int index
                            Layout.fillWidth: true; Layout.fillHeight: true
                            channelIndex: index
                            channel: (Mixer.snapshot.channels || [])[index] || ({})
                        }
                    }
                }
                Text {
                    Layout.fillWidth: true
                    text: window.device.fadersReady ? "Faders are read-only here. Adjust volume on the mixer. Master controls the whole mix."
                        : window.device.connected ? "Waiting for physical fader reports." : "Connect the mixer to use its audio channels."
                    color: Theme.dim; font.pixelSize: 12; wrapMode: Text.WordWrap
                }
            }
            Lighting { settings: window.settings }
            Routing { apps: Mixer.snapshot.apps || [] }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            Text {
                Layout.fillWidth: true
                text: Mixer.error || (Mixer.busy ? "Saving changes..." : "Settings save automatically")
                textFormat: Text.PlainText; wrapMode: Text.WordWrap
                color: Mixer.error ? Theme.danger : Theme.dim; font.pixelSize: 12
            }
            ActionButton {
                visible: Mixer.snapshot.configured === false
                text: "Create settings"; onClicked: Mixer.initialize()
            }
            ActionButton {
                visible: !window.device.connected && window.device.service !== "active" && window.device.service !== "unavailable"
                text: "Start driver"; onClicked: Mixer.startDriver()
            }
        }
    }
}
