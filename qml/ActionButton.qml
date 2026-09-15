import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "Theme.js" as Theme

Button {
    id: control
    property string iconName: ""
    property string tip: text
    property bool primary: false
    property bool selected: false
    property bool compact: false
    property color iconColor: primary ? Theme.accentInk : (selected ? Theme.accent : Theme.text)
    implicitWidth: compact ? 40 : contentItem.implicitWidth + 30
    implicitHeight: compact ? 40 : 42
    padding: compact ? 9 : 13
    hoverEnabled: true
    focusPolicy: Qt.TabFocus
    opacity: enabled ? 1 : 0.35
    Accessible.name: tip
    ToolTip.visible: hovered && tip.length > 0
    ToolTip.text: tip
    ToolTip.delay: 650
    background: Rectangle {
        radius: control.primary && control.compact ? height/2 : 8
        color: control.primary ? (control.down ? "#9cc570" : control.hovered ? "#c5eea0" : Theme.accent)
               : control.selected ? "#283324" : control.down ? "#343a3d" : control.hovered ? Theme.raised : control.flat ? "transparent" : Theme.panel
        border.width: control.activeFocus ? 1 : 0
        border.color: Theme.accent
        Behavior on color { ColorAnimation { duration: 100 } }
    }
    contentItem: RowLayout {
        spacing: control.text.length > 0 && control.iconName.length > 0 ? 9 : 0
        Icon {
            visible: control.iconName.length > 0
            name: control.iconName
            color: control.iconColor
            Layout.preferredWidth: 21
            Layout.preferredHeight: 21
            Layout.alignment: Qt.AlignVCenter | Qt.AlignHCenter
        }
        Text {
            visible: control.text.length > 0
            text: control.text
            textFormat: Text.PlainText
            color: control.iconColor
            font.pixelSize: 13
            font.weight: control.primary ? Font.DemiBold : Font.Normal
            Layout.alignment: Qt.AlignVCenter
        }
    }
}
