pragma Singleton
import QtQuick
QtObject {
    readonly property int cornerRadius: 0
    readonly property color background: "#000000"
    readonly property color panel: "#090909"
    readonly property color border: "#666666"
    readonly property color text: "#ffffff"
    readonly property color dim: "#858585"
    readonly property color accent: "#" + ((Mixer.snapshot.config || {}).color || "00ff40")
    readonly property color danger: "#ff8983"
    function foreground(backgroundColor: color): color {
        function linear(value) { return value <= 0.04045 ? value / 12.92 : Math.pow((value + 0.055) / 1.055, 2.4) }
        const light = 0.2126 * linear(backgroundColor.r) + 0.7152 * linear(backgroundColor.g) + 0.0722 * linear(backgroundColor.b)
        return light > 0.179 ? "#000000" : "#ffffff"
    }
}
