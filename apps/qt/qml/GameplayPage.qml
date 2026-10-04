import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
ColumnLayout {
    id: root
    property bool fullscreen: false
    signal fullscreenRequested()
    signal leaveRequested()
    signal newGameRequested()
    spacing: 14
    Panel {
        Layout.fillWidth: true
        contentItem: RowLayout {
            spacing: 16
            ColumnLayout {
                Layout.fillWidth: true
                Label { text: "AnteaterChess Reborn"; font.pixelSize: 22; font.bold: true; color: Theme.text }
                Label { text: backend.modeText; color: Theme.muted }
            }
            Label { text: backend.turnText; color: Theme.gold; font.pixelSize: 16; font.bold: true }
            ActionButton { text: "New Game"; onClicked: root.newGameRequested() }
            ActionButton { text: root.fullscreen ? "Windowed" : "Fullscreen"; onClicked: root.fullscreenRequested() }
        }
    }
    RowLayout {
        Layout.fillWidth: true; Layout.fillHeight: true
        spacing: 16
        Panel {
            Layout.fillWidth: true; Layout.fillHeight: true
            contentItem: ColumnLayout {
                RowLayout {
                    Layout.fillWidth: true
                    Label { text: backend.blackTimer; color: Theme.text; font.family: "monospace"; Layout.fillWidth: true }
                    Label { text: backend.whiteTimer; color: Theme.text; font.family: "monospace" }
                }
                Board {
                    objectName: "board"
                    Layout.fillWidth: true; Layout.fillHeight: true
                    boardModel: backend.boardModel
                    onSquareClicked: function(row,column,button) { backend.selectSquare(row,column,button) }
                }
                Label { text: "Left click selects · Right click moves · Files A–J, ranks 1–8"; color: Theme.muted; Layout.alignment: Qt.AlignHCenter }
            }
        }
        ColumnLayout {
            Layout.preferredWidth: root.fullscreen ? 380 : 330
            Layout.minimumWidth: 300
            Layout.maximumWidth: root.fullscreen ? 380 : 330
            Layout.fillHeight: true
            spacing: 14
            Panel {
                Layout.fillWidth: true
                contentItem: ColumnLayout {
                    Label { text: "Enter a move"; color: Theme.text; font.bold: true }
                    RowLayout {
                        MoveField {
                            id: from; objectName: "fromEntry"; placeholderText: "From · E2"
                            Layout.fillWidth: true; enabled: backend.humanTurn
                            Layout.minimumWidth: 0
                            text: backend.fromText; validInput: backend.fromValid
                            onTextEdited: backend.setMoveFields(text,to.text)
                            onAccepted: backend.submitFields()
                        }
                        MoveField {
                            id: to; objectName: "toEntry"; placeholderText: "To · E4"
                            Layout.fillWidth: true; enabled: backend.humanTurn
                            Layout.minimumWidth: 0
                            text: backend.toText; validInput: backend.toValid
                            onTextEdited: backend.setMoveFields(from.text,text)
                            onAccepted: backend.submitFields()
                        }
                    }
                    ActionButton { objectName: "submitMove"; text: backend.humanTurn ? "Submit Move" : "AI thinking…"; primary: true; enabled: backend.humanTurn; Layout.fillWidth: true; onClicked: backend.submitFields() }
                    RowLayout {
                        ActionButton { text: "Undo"; enabled: backend.canUndo; Layout.fillWidth: true; onClicked: backend.undo() }
                        ActionButton { text: "Hint"; enabled: backend.canHint; Layout.fillWidth: true; onClicked: backend.hint() }
                    }
                }
            }
            Panel {
                Layout.fillWidth: true; Layout.fillHeight: true
                contentItem: ColumnLayout {
                    RowLayout {
                        Image { source: "qrc:/org/anteater/reborn/icon-history-dark.svg"; sourceSize.width: 20; sourceSize.height: 20 }
                        Label { text: "Move History"; color: Theme.text; font.bold: true; Layout.fillWidth: true }
                        Label { text: backend.clockText; color: Theme.gold; font.family: "monospace" }
                    }
                    Label { text: backend.aiSummary; color: Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    ListView {
                        id: history; objectName: "historyView"
                        Layout.fillWidth: true; Layout.fillHeight: true
                        clip: true; model: backend.historyModel
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Label {
                            required property string moveText
                            text: moveText; width: history.width; wrapMode: Text.Wrap
                            color: Theme.text; font.family: "monospace"; font.pixelSize: root.fullscreen ? 16 : 13
                            padding: 5
                        }
                        // Follow new moves only if the reader was already at the end.
                        property bool followEnd: true
                        onMovementEnded: followEnd = atYEnd
                        onCountChanged: if (followEnd) Qt.callLater(positionViewAtEnd)
                    }
                }
            }
            Panel {
                Layout.fillWidth: true
                contentItem: ColumnLayout {
                    RowLayout {
                        Image { source: backend.statusError ? "qrc:/org/anteater/reborn/icon-alert-dark.svg" : "qrc:/org/anteater/reborn/icon-info-dark.svg"; sourceSize.width: 20; sourceSize.height: 20 }
                        Label { text: backend.status; color: backend.statusError ? "#fee2e2" : backend.busy ? "#dcfce7" : Theme.muted; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    }
                    ActionButton { text: "End Game"; destructive: true; Layout.fillWidth: true; onClicked: root.leaveRequested() }
                }
            }
        }
    }
}
