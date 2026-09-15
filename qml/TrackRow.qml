import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "Theme.js" as Theme

Rectangle {
    id: row
    property string title: ""
    property string subtitle: ""
    property string fileType: ""
    property string path: ""
    property int number: 0
    property bool current: false
    property bool selected: false
    property int rowIndex: -1
    signal selectedRow(bool extend)
    signal activated()
    height: 58
    radius: 7
    color: selected ? "#2b3527" : current ? "#202b21" : mouse.containsMouse ? "#212629" : "transparent"
    Accessible.role: Accessible.ListItem
    Accessible.name: title + ", " + subtitle
    Accessible.selected: selected
    Accessible.onPressAction: selectedRow(false)
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 16
        spacing: 16
        Item {
            Layout.preferredWidth: 28
            Layout.fillHeight: true
            Text {
                anchors.centerIn: parent
                visible: !row.current && !mouse.containsMouse
                text: row.number < 10 ? "0"+row.number : row.number
                color: Theme.muted; font.pixelSize: 12
            }
            Icon { anchors.centerIn: parent; visible: row.current || mouse.containsMouse; name: row.current ? "music" : "play"; color: Theme.accent; width: 18; height: 18 }
        }
        Text {
            Layout.fillWidth: true
            text: row.title; textFormat: Text.PlainText; elide: Text.ElideRight
            color: row.current ? Theme.accent : Theme.text
            font.pixelSize: 14; font.weight: row.current ? Font.DemiBold : Font.Normal
        }
        Text {
            Layout.preferredWidth: row.width > 650 ? 170 : 115
            text: row.subtitle; textFormat: Text.PlainText; elide: Text.ElideRight
            color: Theme.muted; font.pixelSize: 12
        }
        Rectangle {
            Layout.preferredWidth: 54; Layout.preferredHeight: 24
            radius: 5; color: "#252c2d"; border.color: "#32393a"
            Text { anchors.centerIn: parent; text: row.fileType; textFormat: Text.PlainText; color: "#a2b0aa"; font.pixelSize: 10; font.letterSpacing: 0.7 }
        }
    }
    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        onClicked: function(event) {
            row.selectedRow((event.modifiers & Qt.ControlModifier) !== 0)
            if(event.button === Qt.RightButton) contextMenu.popup()
        }
        onDoubleClicked: row.activated()
        cursorShape: Qt.PointingHandCursor
    }
    ToolTip.visible: mouse.containsMouse
    ToolTip.text: path
    ToolTip.delay: 1400
    Menu {
        id: contextMenu
        MenuItem { text: qsTr("播放"); onTriggered: row.activated() }
        MenuItem { text: qsTr("从列表移除"); onTriggered: libraryController.removeSelected() }
    }
}
