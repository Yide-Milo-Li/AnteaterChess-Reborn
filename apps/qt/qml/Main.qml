import AnteaterChess.Reborn 1.0
import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts
ApplicationWindow {
    id: window
    objectName: "mainWindow"
    visible: true
    width: 1200; height: 850
    minimumWidth: 900; minimumHeight: 700
    title: "AnteaterChess Reborn"
    color: Theme.background
    required property ApplicationController controller
    property bool isFullscreen: visibility === Window.FullScreen
    property int windowedVisibility: Window.Windowed
    font.family: "Segoe UI"
    palette.window: Theme.background
    palette.windowText: Theme.text
    palette.text: Theme.text
    palette.buttonText: Theme.text
    palette.button: "#1f2430"
    palette.base: Theme.inset
    palette.highlight: Theme.gold
    function restoreWindowed() {
        if (windowedVisibility === Window.Maximized) showMaximized()
        else showNormal()
    }
    function toggleFullscreen() {
        if (isFullscreen) restoreWindowed()
        else {
            // Fullscreen must retain the user's maximized/ordinary window choice.
            windowedVisibility = visibility === Window.Maximized ? Window.Maximized : Window.Windowed
            showFullScreen()
        }
    }
    Shortcut { sequence: "F11"; onActivated: window.toggleFullscreen() }
    Shortcut { sequence: "Escape"; enabled: window.isFullscreen; onActivated: window.restoreWindowed() }
    onClosing: function(close) { close.accepted = controller.requestClose() }
    Connections {
        target: controller
        function onCloseReady() { window.close() }
    }
    Loader {
        id: pages
        objectName: "pageLoader"
        anchors.fill: parent; anchors.margins: 24
        sourceComponent: controller.page === ApplicationController.MainMenu ? menu : controller.page === ApplicationController.ModeMenu ? modes : controller.page === ApplicationController.Setup ? setup : controller.page === ApplicationController.Gameplay ? game : ended
    }
    Component {
        id: menu
        Item {
            Panel {
                anchors.centerIn: parent; width: 430
                contentItem: ColumnLayout {
                    spacing: 20
                    Label { text: "AnteaterChess Reborn"; color: Theme.text; font.pixelSize: 26; font.bold: true; Layout.alignment: Qt.AlignHCenter }
                    Label { text: "8 × 10 · Ants & Anteaters"; color: Theme.muted; Layout.alignment: Qt.AlignHCenter }
                    ActionButton { objectName: "newGame"; text: "New Game"; primary: true; Layout.fillWidth: true; onClicked: controller.newGame() }
                    ActionButton { text: "Quit Game"; Layout.fillWidth: true; onClicked: { confirmation.action = "quit"; confirmation.open() } }
                    Label { text: "Team 22 · DeepAnteater"; color: Theme.muted; Layout.alignment: Qt.AlignHCenter }
                }
            }
        }
    }
    Component {
        id: modes
        Item {
            Panel {
                anchors.centerIn: parent; width: 430
                contentItem: ColumnLayout {
                    spacing: 16
                    Label { text: "Choose Game Mode"; color: Theme.text; font.pixelSize: 22; font.bold: true }
                    Repeater {
                        model: [{text:"Human vs Human",value:Game.HumanVsHuman},{text:"Human vs AI",value:Game.HumanVsComputer},{text:"AI vs AI",value:Game.ComputerVsComputer}]
                        ActionButton {
                            required property int index
                            required property var modelData
                            text: modelData.text; Layout.fillWidth: true
                            onClicked: controller.chooseMode(modelData.value)
                        }
                    }
                    ActionButton { text: "Back"; Layout.fillWidth: true; onClicked: controller.back() }
                }
            }
        }
    }
    Component {
        id: setup
        SetupPage {
            controller: window.controller
            onStartRequested: window.controller.startDraft()
            onBackRequested: controller.back()
        }
    }
    Component {
        id: game
        GameplayPage {
            controller: window.controller
            fullscreen: window.isFullscreen
            onFullscreenRequested: window.toggleFullscreen()
            onLeaveRequested: { confirmation.action = "finish"; confirmation.open() }
            onNewGameRequested: { confirmation.action = "new"; confirmation.open() }
        }
    }
    Component {
        id: ended
        Item {
            Panel {
                anchors.centerIn: parent; width: 430
                contentItem: ColumnLayout {
                    spacing: 18
                    Label { text: controller.game.resultText; color: Theme.gold; font.pixelSize: 28; font.bold: true }
                    Label { text: controller.game.historyCount+" half-moves · "+controller.clocks.elapsed; color: Theme.muted }
                    Label { text: controller.game.error ? controller.game.message : ""; color: "#fee2e2"; wrapMode: Text.Wrap; Layout.fillWidth: true }
                    ActionButton { text: "New Game"; primary: true; Layout.fillWidth: true; onClicked: controller.newGame() }
                    ActionButton { text: "Main Menu"; Layout.fillWidth: true; onClicked: controller.back() }
                    ActionButton { text: "Exit"; Layout.fillWidth: true; onClicked: { confirmation.action="quit"; confirmation.open() } }
                }
            }
        }
    }
    Dialog {
        id: confirmation
        objectName: "confirmationDialog"
        property string action: ""
        anchors.centerIn: parent; modal: true
        title: action === "quit" ? "Quit Game" : action === "new" ? "New Game" : "Leave Game"
        footer: DialogButtonBox {
            standardButtons: Dialog.Yes | Dialog.No
            spacing: 8; padding: 12
            delegate: ActionButton { }
            background: Rectangle { color: Theme.inset }
            onAccepted: confirmation.accept()
            onRejected: confirmation.reject()
        }
        Label { text: "Are you sure?"; color: Theme.text }
        onAccepted: { if (action === "quit") window.close(); else if (action === "new") controller.newGame(); else controller.finish() }
    }
    Dialog {
        id: promotion
        objectName: "promotionDialog"
        anchors.centerIn: parent; modal: true; title: "Choose promotion piece"
        ColumnLayout {
            Repeater {
                model: [{text:"Queen",value:Game.PromoteQueen},{text:"Rook",value:Game.PromoteRook},{text:"Bishop",value:Game.PromoteBishop},{text:"Knight",value:Game.PromoteKnight}]
                ActionButton {
                    required property int index
                    required property var modelData
                    text: modelData.text; Layout.fillWidth: true
                    onClicked: { controller.submitFields(modelData.value); promotion.close() }
                }
            }
            ActionButton { objectName: "promotionCancel"; text: "Cancel"; Layout.fillWidth: true; onClicked: { controller.cancelPromotion(); promotion.close() } }
        }
        onRejected: controller.cancelPromotion()
    }
    Connections {
        target: controller
        function onPromotionRequested() { promotion.open() }
        function onPromotionDismissed() { promotion.close() }
    }
}
