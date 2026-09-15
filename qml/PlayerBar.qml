import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml 2.15
import "Theme.js" as Theme

Rectangle {
    id: bar
    implicitHeight: 114
    color: "#1b1f20"
    Rectangle { width: parent.width; height: 1; color: Theme.border }
    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 18
        RecordArtwork { Layout.preferredWidth: 54; Layout.preferredHeight: 54; visible: bar.width > 980 }
        ColumnLayout {
            Layout.preferredWidth: bar.width > 1100 ? 225 : 155
            Layout.maximumWidth: 250
            spacing: 7
            Text { Layout.fillWidth: true; text: libraryController.title; textFormat: Text.PlainText; color: Theme.text; font.pixelSize: 13; font.weight: Font.DemiBold; elide: Text.ElideRight }
            Text { Layout.fillWidth: true; text: libraryController.artist; textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: 11; elide: Text.ElideRight }
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 3
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 14
                ActionButton {
                    objectName: "modeButton"
                    compact: true; flat: true; iconName: ["sequential","repeat","repeatOne","shuffle"][libraryController.playMode]
                    iconColor: Theme.accent; tip: libraryController.modeText + qsTr(" · 点击切换")
                    onClicked: libraryController.cycleMode()
                }
                ActionButton { objectName: "previousButton"; compact: true; flat: true; iconName: "previous"; tip: qsTr("上一首（Ctrl+Left）"); enabled: libraryController.trackCount > 0; onClicked: libraryController.previous() }
                ActionButton {
                    objectName: "playButton"
                    compact: true; primary: true
                    Layout.preferredWidth: 46; Layout.preferredHeight: 46
                    iconName: libraryController.playing ? "pause" : "play"
                    tip: libraryController.playing ? qsTr("暂停（空格）") : qsTr("播放（空格）")
                    enabled: libraryController.trackCount > 0 && !libraryController.loading
                    onClicked: libraryController.togglePlayback()
                }
                ActionButton { objectName: "nextButton"; compact: true; flat: true; iconName: "next"; tip: qsTr("下一首（Ctrl+Right）"); enabled: libraryController.trackCount > 0; onClicked: libraryController.next() }
                ActionButton { compact: true; flat: true; iconName: "locate"; tip: qsTr("定位当前曲目"); enabled: libraryController.trackCount > 0; onClicked: libraryController.revealCurrent() }
            }
            RowLayout {
                spacing: 7
                Text { text: libraryController.formatTime(progress.pressed ? progress.value : libraryController.position); color: Theme.muted; font.pixelSize: 10; Layout.minimumWidth: 38 }
                PlayerSlider {
                    id: progress
                    objectName: "progressSlider"
                    Layout.fillWidth: true
                    from: 0; to: Math.max(1,libraryController.duration)
                    stepSize: 1000
                    enabled: libraryController.canSeek
                    Accessible.name: qsTr("播放进度")
                    onPressedChanged: if(!pressed) libraryController.seek(Math.round(value))
                    onMoved: if(!pressed) libraryController.seek(Math.round(value))
                    Binding { target: progress; property: "value"; value: libraryController.position; when: !progress.pressed; restoreMode: Binding.RestoreBinding }
                }
                Text { text: libraryController.formatTime(libraryController.duration); color: Theme.muted; font.pixelSize: 10; Layout.minimumWidth: 38 }
            }
        }
        RowLayout {
            Layout.preferredWidth: 152
            spacing: 3
            ActionButton { objectName: "muteButton"; compact: true; flat: true; iconName: libraryController.volume === 0 ? "muted" : "volume"; tip: qsTr("静音 / 恢复（M）"); onClicked: libraryController.toggleMute() }
            PlayerSlider {
                id: volume
                objectName: "volumeSlider"
                Layout.preferredWidth: 82
                from: 0; to: 100; stepSize: 1
                Accessible.name: qsTr("音量")
                onMoved: libraryController.volume = Math.round(value)
                Binding { target: volume; property: "value"; value: libraryController.volume; when: !volume.pressed; restoreMode: Binding.RestoreBinding }
            }
            Text { Layout.preferredWidth: 26; text: libraryController.volume; color: Theme.muted; font.pixelSize: 10 }
        }
    }
}
