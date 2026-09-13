import QtQuick
import QtQuick.Controls
import RazerMixer
Button {
    id: root
    property bool emphasized: false
    property color selectedColor: Theme.accent
    implicitHeight: 30
    implicitWidth: Math.max(72, contentItem.implicitWidth + 20)
    padding: 6
    font.pixelSize: 12
    contentItem: Text {
        text: root.text
        font: root.font
        color: !root.enabled ? Theme.dim : root.emphasized || root.checked ? Theme.foreground(root.selectedColor) : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: Theme.cornerRadius
        color: root.emphasized || root.checked ? root.selectedColor : root.hovered ? "#141414" : Theme.panel
        border.color: root.activeFocus ? Theme.accent : Theme.border
        border.width: root.activeFocus ? 2 : 1
        opacity: root.enabled ? 1 : 0.5
    }
}
