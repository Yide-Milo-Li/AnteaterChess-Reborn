import AnteaterChess.Reborn 1.0
import QtQuick
import QtQuick.Controls
Button {
    id: control
    property bool primary: false
    property bool destructive: false
    implicitHeight: 42
    implicitWidth: Math.max(100, contentItem.implicitWidth + 32)
    font.pixelSize: 13
    font.bold: true
    contentItem: Text {
        text: control.text
        font: control.font
        color: !control.enabled ? "#64748b" : control.primary ? "#0b0d13" : control.destructive ? "#fee2e2" : Theme.text
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 8
        color: !control.enabled ? "#12151d" : control.primary ? Theme.gold : control.destructive ? "#381616" : control.down ? "#323b4e" : control.hovered ? "#282f3f" : "#1f2430"
        border.color: control.activeFocus || control.hovered ? Theme.gold : control.destructive ? "#7f1d1d" : "#333c4f"
    }
}
