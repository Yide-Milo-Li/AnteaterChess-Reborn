import QtQuick
import QtQuick.Window
Item {
    id: root
    property var boardModel
    signal squareClicked(int row, int column, int button)
    readonly property real cellSize: Math.max(24, Math.floor(Math.min((width-52)/10, (height-52)/8)))
    Rectangle {
        width: root.cellSize*10+52; height: root.cellSize*8+52
        anchors.centerIn: parent
        color: "#0a0c10"; radius: 10; border.color: "#4a3828"; border.width: 2
        Grid {
            x: 26; y: 26; columns: 10
            Repeater {
                model: root.boardModel
                delegate: Rectangle {
                    required property int boardRow
                    required property int boardColumn
                    required property string coordinate
                    required property string asset
                    required property bool selected
                    required property bool legal
                    required property bool hintFrom
                    required property bool hintTo
                    objectName: "square_"+boardRow+"_"+boardColumn
                    width: root.cellSize; height: root.cellSize
                    color: (boardRow+boardColumn)%2 ? Theme.walnut : Theme.ivory
                    Image {
                        anchors.centerIn: parent
                        width: parent.width*0.84; height: parent.height*0.84
                        source: asset
                        fillMode: Image.PreserveAspectFit
                        sourceSize.width: Math.ceil(width*Screen.devicePixelRatio)
                        sourceSize.height: Math.ceil(height*Screen.devicePixelRatio)
                        smooth: true
                    }
                    Rectangle {
                        anchors.fill: parent; color: "transparent"
                        border.width: selected || legal || hintFrom || hintTo ? 3 : 0
                        border.color: selected ? "#f59e0b" : hintFrom ? "#06b6d4" : hintTo ? "#eab308" : "#10b981"
                    }
                    MouseArea {
                        anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        onClicked: function(mouse) { root.squareClicked(boardRow,boardColumn,mouse.button) }
                    }
                    Accessible.name: coordinate
                    Accessible.role: Accessible.Button
                }
            }
        }
        Repeater {
            model: 10
            Text {
                required property int index
                x: 26+index*root.cellSize; y: 8*root.cellSize+30
                width: root.cellSize; height: 20; horizontalAlignment: Text.AlignHCenter
                text: String.fromCharCode(65+index); color: Theme.muted; font.family: "monospace"
            }
        }
        Repeater {
            model: 8
            Text {
                required property int index
                x: 4; y: 26+index*root.cellSize; width: 20; height: root.cellSize
                verticalAlignment: Text.AlignVCenter; horizontalAlignment: Text.AlignHCenter
                text: 8-index; color: Theme.muted; font.family: "monospace"
            }
        }
    }
}
