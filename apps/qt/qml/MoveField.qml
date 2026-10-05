import AnteaterChess.Reborn 1.0
import QtQuick
import QtQuick.Controls
TextField {
    id: field
    property bool validInput: false
    implicitHeight: 40
    color: Theme.text
    placeholderTextColor: Theme.muted
    font.family: "monospace"
    font.pixelSize: 14
    selectByMouse: true
    background: Rectangle {
        color: "#0b0d13"; radius: 6
        border.color: field.activeFocus ? Theme.gold : field.text.length ? field.validInput ? "#10b981" : "#ef4444" : "#2e374a"
    }
}
