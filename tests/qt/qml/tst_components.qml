import QtQuick
import QtTest
import "qrc:/qml" as UI
TestCase {
    name: "DesktopComponents"
    when: windowShown
    visible: true
    width: 900; height: 700
    UI.SetupPage { id: setup; width: 900; height: 700; visible: false; mode: 2 }
    UI.Board { id: board; width: 800; height: 640; boardModel: backend.boardModel }
    SignalSpy { id: clicked; target: board; signalName: "squareClicked" }
    function test_difficultyValues() {
        compare(setup.difficulties.length,4)
        compare(setup.difficultyValue(0),1)
        compare(setup.difficultyValue(1),2)
        compare(setup.difficultyValue(2),3)
        compare(setup.difficultyValue(3),5)
    }
    function test_coordinateMouseMapping() {
        clicked.clear()
        var square = findChild(board,"square_6_4")
        verify(square !== null)
        verify(square.visible)
        mouseClick(square,square.width/2,square.height/2,Qt.LeftButton)
        tryCompare(clicked,"count",1)
        compare(clicked.signalArguments[0][0],6)
        compare(clicked.signalArguments[0][1],4)
        compare(clicked.signalArguments[0][2],Qt.LeftButton)
        mouseClick(square,square.width/2,square.height/2,Qt.RightButton)
        compare(clicked.signalArguments[1][2],Qt.RightButton)
    }
}
