import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import RazerMixer

ScrollView {
    id: root
    required property var apps
    clip: true
    contentWidth: availableWidth
    ColumnLayout {
        width: root.availableWidth; spacing: 14
        Text { Layout.fillWidth: true; text: "Choose where each app plays. Master volume controls the whole mix."; color: Theme.dim; font.pixelSize: 13; wrapMode: Text.WordWrap }
        Repeater {
            model: root.apps
            Rectangle {
                id: row
                required property var modelData
                Layout.fillWidth: true; implicitHeight: 82
                color: Theme.panel; radius: Theme.cornerRadius; border.color: Theme.border
                RowLayout {
                    anchors.fill: parent; anchors.margins: 18; spacing: 12
                    ColumnLayout {
                        Layout.fillWidth: true; spacing: 6
                        Text { Layout.fillWidth: true; text: row.modelData.name; textFormat: Text.PlainText; color: Theme.text; font.pixelSize: 16; elide: Text.ElideRight }
                        Text { text: row.modelData.running ? "Playing audio" : "Applies when the app plays audio"; color: Theme.dim; font.pixelSize: 11 }
                    }
                    Repeater {
                        model: [{id:"master",label:"System"},{id:"chat",label:"Chat"},{id:"music",label:"Music"}]
                        ActionButton {
                            required property var modelData
                            text: modelData.label; checked: row.modelData.channel === modelData.id
                            onClicked: Mixer.route(row.modelData.id, modelData.id)
                        }
                    }
                }
            }
        }
        Text { Layout.fillWidth: true; text: "Other apps appear while playing audio through the mixer. Saved assignments remain here."; color: Theme.dim; font.pixelSize: 12; wrapMode: Text.WordWrap }
    }
}
