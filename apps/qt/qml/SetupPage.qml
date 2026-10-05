import AnteaterChess.Reborn 1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Item {
    id: root
    required property ApplicationController controller
    readonly property int mode: controller.settings.mode
    signal startRequested()
    signal backRequested()
    readonly property var difficulties: [{text:"Easy",value:Game.Easy},{text:"Medium",value:Game.Medium},{text:"Hard",value:Game.Hard},{text:"Tournament",value:Game.Tournament}]
    Panel {
        anchors.centerIn: parent; width: Math.min(parent.width,620)
        contentItem: ColumnLayout {
            spacing: 16
            Label { text: "Game Setup"; color: Theme.text; font.pixelSize: 24; font.bold: true }
            Label { text: ["Human vs Human","Human vs AI","AI vs AI"][root.mode]; color: Theme.muted }
            RowLayout {
                visible: root.mode === Game.HumanVsComputer
                Label { text: "Your side"; color: Theme.text; Layout.fillWidth: true }
                ComboBox { id: side; objectName: "playerSide"; textRole: "text"
                    model: [{text:"White",value:Game.White},{text:"Black",value:Game.Black}]
                    currentIndex: controller.settings.playerColor === Game.White ? 0 : 1
                    onActivated: controller.settings.playerColor = model[currentIndex].value }
            }
            RowLayout {
                visible: root.mode === Game.ComputerVsComputer || (root.mode === Game.HumanVsComputer && controller.settings.playerColor === Game.Black)
                Label { text: "White AI"; color: Theme.text; Layout.fillWidth: true }
                ComboBox { id: white; objectName: "whiteDifficulty"; textRole: "text"; model: root.difficulties
                    currentIndex: root.difficulties.findIndex(item => item.value === controller.settings.whiteDifficulty)
                    onActivated: controller.settings.whiteDifficulty = model[currentIndex].value }
            }
            RowLayout {
                visible: root.mode === Game.ComputerVsComputer || (root.mode === Game.HumanVsComputer && controller.settings.playerColor === Game.White)
                Label { text: "Black AI"; color: Theme.text; Layout.fillWidth: true }
                ComboBox { id: black; objectName: "blackDifficulty"; textRole: "text"; model: root.difficulties
                    currentIndex: root.difficulties.findIndex(item => item.value === controller.settings.blackDifficulty)
                    onActivated: controller.settings.blackDifficulty = model[currentIndex].value }
            }
            CheckBox { id: timed; text: "Enable turn timer"; checked: controller.settings.timerEnabled; onToggled: controller.settings.timerEnabled = checked }
            RowLayout {
                visible: timed.checked
                Label { text: "Hours"; color: Theme.muted }
                SpinBox { id: hours; from: 0; to: 1; editable: true; value: Math.floor(controller.settings.turnSeconds/3600); onValueModified: controller.settings.turnSeconds = hours.value*3600+minutes.value*60+seconds.value }
                Label { text: "Minutes"; color: Theme.muted }
                SpinBox { id: minutes; from: 0; to: 59; editable: true; value: Math.floor(controller.settings.turnSeconds/60)%60; onValueModified: controller.settings.turnSeconds = hours.value*3600+minutes.value*60+seconds.value }
                Label { text: "Seconds"; color: Theme.muted }
                SpinBox { id: seconds; from: 0; to: 59; editable: true; value: controller.settings.turnSeconds%60; onValueModified: controller.settings.turnSeconds = hours.value*3600+minutes.value*60+seconds.value }
            }
            RowLayout {
                visible: root.mode !== Game.HumanVsHuman
                Label { text: "AI budget override (seconds, 0 = default)"; color: Theme.text; Layout.fillWidth: true }
                SpinBox { id: budget; from: 0; to: 3600; editable: true; value: controller.settings.aiSeconds; onValueModified: controller.settings.aiSeconds = value }
            }
            Label {
                text: "Easy 350 ms · Medium 2200 ms · Hard 7000 ms\nTournament: 10:00.999 per side, at most 10000 ms per move.\nTurn expiry skips a turn. Undo does not refund Tournament time."
                color: Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true
            }
            Label { text: controller.game.error ? controller.game.message : ""; color: "#fee2e2"; wrapMode: Text.Wrap; Layout.fillWidth: true }
            RowLayout {
                ActionButton { text: "Back"; Layout.fillWidth: true; onClicked: root.backRequested() }
                ActionButton {
                    objectName: "startGame"; text: "Start Game"; primary: true; Layout.fillWidth: true
                    onClicked: root.startRequested()
                }
            }
        }
    }
}
