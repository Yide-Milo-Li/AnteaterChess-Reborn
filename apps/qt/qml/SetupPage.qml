import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    property int mode: 0
    signal startRequested(int color, int white, int black, bool timer, int seconds, int budget)
    signal backRequested()
    readonly property var difficulties: ["Easy","Medium","Hard","Tournament"]
    function difficultyValue(index) { return index === 3 ? 5 : index+1 }
    Panel {
        anchors.centerIn: parent; width: Math.min(parent.width,620)
        contentItem: ColumnLayout {
            spacing: 16
            Label { text: "Game Setup"; color: Theme.text; font.pixelSize: 24; font.bold: true }
            Label { text: ["Human vs Human","Human vs AI","AI vs AI"][root.mode]; color: Theme.muted }
            RowLayout {
                visible: root.mode === 1
                Label { text: "Your side"; color: Theme.text; Layout.fillWidth: true }
                ComboBox { id: side; objectName: "playerSide"; model: ["White","Black"] }
            }
            RowLayout {
                visible: root.mode === 2 || (root.mode === 1 && side.currentIndex === 1)
                Label { text: "White AI"; color: Theme.text; Layout.fillWidth: true }
                ComboBox { id: white; objectName: "whiteDifficulty"; model: root.difficulties }
            }
            RowLayout {
                visible: root.mode === 2 || (root.mode === 1 && side.currentIndex === 0)
                Label { text: "Black AI"; color: Theme.text; Layout.fillWidth: true }
                ComboBox { id: black; objectName: "blackDifficulty"; model: root.difficulties }
            }
            CheckBox { id: timed; text: "Enable turn timer" }
            RowLayout {
                visible: timed.checked
                Label { text: "Hours"; color: Theme.muted }
                SpinBox { id: hours; from: 0; to: 1; editable: true }
                Label { text: "Minutes"; color: Theme.muted }
                SpinBox { id: minutes; from: 0; to: 59; editable: true }
                Label { text: "Seconds"; color: Theme.muted }
                SpinBox { id: seconds; from: 0; to: 59; editable: true }
            }
            RowLayout {
                visible: root.mode !== 0
                Label { text: "AI budget override (seconds, 0 = default)"; color: Theme.text; Layout.fillWidth: true }
                SpinBox { id: budget; from: 0; to: 3600; editable: true }
            }
            Label {
                text: "Easy 350 ms · Medium 2200 ms · Hard 7000 ms\nTournament: 10:00.999 per side, at most 10000 ms per move.\nTurn expiry skips a turn. Undo does not refund Tournament time."
                color: Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true
            }
            Label { text: backend.statusError ? backend.status : ""; color: "#fee2e2"; wrapMode: Text.Wrap; Layout.fillWidth: true }
            RowLayout {
                ActionButton { text: "Back"; Layout.fillWidth: true; onClicked: root.backRequested() }
                ActionButton {
                    objectName: "startGame"; text: "Start Game"; primary: true; Layout.fillWidth: true
                    onClicked: root.startRequested(side.currentIndex,root.difficultyValue(white.currentIndex),root.difficultyValue(black.currentIndex),timed.checked,hours.value*3600+minutes.value*60+seconds.value,budget.value)
                }
            }
        }
    }
}
