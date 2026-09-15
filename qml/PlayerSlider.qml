import QtQuick 2.15
import QtQuick.Controls 2.15
import "Theme.js" as Theme
Slider {
    id: slider
    implicitHeight: 24
    padding: 7
    hoverEnabled: true
    background: Rectangle {
        x: slider.leftPadding
        y: slider.topPadding + slider.availableHeight/2 - height/2
        width: slider.availableWidth
        height: 4
        radius: 2
        color: "#373e40"
        Rectangle { width: slider.visualPosition * parent.width; height: parent.height; radius: 2; color: slider.enabled ? Theme.accent : Theme.muted }
    }
    handle: Rectangle {
        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth-width)
        y: slider.topPadding + slider.availableHeight/2-height/2
        width: 12; height: 12; radius: 6
        color: Theme.accent
        visible: slider.enabled && (slider.hovered || slider.pressed || slider.activeFocus)
        border.width: slider.activeFocus ? 2 : 0
        border.color: Theme.text
    }
}
