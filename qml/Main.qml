import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import "Theme.js" as Theme

ApplicationWindow {
    id: window
    objectName: "mainWindow"
    visible: true
    width: libraryController.savedWidth
    height: libraryController.savedHeight
    minimumWidth: 880
    minimumHeight: 640
    title: qsTr("声间 · 本地音乐播放器")
    color: Theme.background
    palette.window: Theme.panel
    palette.windowText: Theme.text
    palette.base: Theme.background
    palette.text: Theme.text
    palette.button: Theme.raised
    palette.buttonText: Theme.text
    palette.highlight: "#34462c"
    palette.highlightedText: Theme.text
    property bool textEditing: search.activeFocus
    onClosing: if(visibility !== Window.Maximized) libraryController.saveWindowSize(width,height)

    Shortcut { sequence: "Ctrl+O"; onActivated: libraryController.addFiles() }
    Shortcut { sequence: "Ctrl+Shift+O"; onActivated: libraryController.addFolder() }
    Shortcut { sequence: "Ctrl+F"; onActivated: search.forceActiveFocus() }
    Shortcut { sequence: "Space"; enabled: !window.textEditing && !helpDialog.opened; onActivated: libraryController.togglePlayback() }
    Shortcut { sequence: "M"; enabled: !window.textEditing && !helpDialog.opened; onActivated: libraryController.toggleMute() }
    Shortcut { sequence: "Ctrl+Right"; onActivated: libraryController.next() }
    Shortcut { sequence: "Ctrl+Left"; onActivated: libraryController.previous() }
    Shortcut { sequence: "Delete"; enabled: !window.textEditing && !helpDialog.opened; onActivated: libraryController.removeSelected() }
    Shortcut { sequence: "Escape"; enabled: !helpDialog.opened; onActivated: { libraryController.searchText=""; tracks.forceActiveFocus() } }

    Item {
        anchors.fill: parent
        Item {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: playbackBar.top
            Rectangle {
                id: sidebar
                width: window.width > 1000 ? 196 : 176
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                color: Theme.sidebar
                Rectangle { anchors.right: parent.right; height: parent.height; width: 1; color: "#202428" }
                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: 32; anchors.bottomMargin: 24
                    anchors.leftMargin: 20; anchors.rightMargin: 20
                    spacing: 12
                    RowLayout {
                        spacing: 10
                        Icon { name: "music"; color: Theme.accent; Layout.preferredWidth: 27; Layout.preferredHeight: 27 }
                        Text { text: qsTr("声间"); color: Theme.text; font.pixelSize: 25; font.weight: Font.DemiBold; font.letterSpacing: 3 }
                    }
                    Text { text: "A SPACE FOR MUSIC"; color: "#6d7775"; font.pixelSize: 8; font.letterSpacing: 1.5 }
                    Item { Layout.preferredHeight: 29 }
                    Text { text: qsTr("音乐库"); color: Theme.muted; font.pixelSize: 10; font.letterSpacing: 2 }
                    ActionButton { Layout.fillWidth: true; text: qsTr("我的音乐"); iconName: "music"; selected: true; onClicked: { libraryController.searchText=""; tracks.forceActiveFocus() } }
                    Text { text: qsTr("%1 首本地音乐").arg(libraryController.trackCount); color: "#73806f"; font.pixelSize: 11; Layout.leftMargin: 14 }
                    Item { Layout.preferredHeight: 14 }
                    Text { text: qsTr("收藏管理"); color: Theme.muted; font.pixelSize: 10; font.letterSpacing: 2 }
                    ActionButton { Layout.fillWidth: true; flat: true; text: qsTr("添加音频"); iconName: "add"; onClicked: libraryController.addFiles() }
                    ActionButton { Layout.fillWidth: true; flat: true; text: qsTr("导入文件夹"); iconName: "folder"; tip: qsTr("递归导入文件夹（Ctrl+Shift+O）"); onClicked: libraryController.addFolder() }
                    Item { Layout.fillHeight: true }
                    Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border }
                    RowLayout {
                        spacing: 7
                        Rectangle { width: 5; height: 5; radius: 3; color: Theme.accent }
                        Text { text: qsTr("本地音乐，随时聆听"); color: Theme.muted; font.pixelSize: 10 }
                    }
                    ActionButton { text: qsTr("使用帮助  ↗"); flat: true; onClicked: helpDialog.open() }
                }
            }
            ColumnLayout {
                anchors.left: sidebar.right
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.margins: window.width > 1000 ? 30 : 22
                spacing: 20
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 20
                    ColumnLayout {
                        spacing: 6
                        Text { text: qsTr("我的音乐"); color: Theme.text; font.pixelSize: 28; font.weight: Font.DemiBold }
                        Text { text: qsTr("把喜欢的声音，留在身边。"); color: Theme.muted; font.pixelSize: 12 }
                    }
                    Item { Layout.fillWidth: true }
                    ActionButton { objectName: "addFilesButton"; primary: true; text: qsTr("添加音乐"); iconName: "add"; tip: qsTr("添加音乐（Ctrl+O）"); onClicked: libraryController.addFiles() }
                }
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: window.height > 700 ? 176 : 146
                    radius: 12
                    gradient: Gradient {
                        orientation: Gradient.Horizontal
                        GradientStop { position: 0; color: "#252f26" }
                        GradientStop { position: 1; color: "#1d2321" }
                    }
                    border.color: "#323d31"
                    RowLayout {
                        anchors.fill: parent; anchors.margins: window.height > 700 ? 22 : 18; spacing: 25
                        RecordArtwork {
                            Layout.preferredWidth: window.height > 700 ? 132 : 102
                            Layout.preferredHeight: window.height > 700 ? 132 : 102
                        }
                        ColumnLayout {
                            Layout.fillWidth: true; Layout.fillHeight: true; spacing: window.height > 700 ? 8 : 6
                            RowLayout {
                                Text { text: "NOW PLAYING"; color: "#a9bd9a"; font.pixelSize: 9; font.letterSpacing: 2 }
                                Item { Layout.fillWidth: true }
                                Rectangle {
                                    implicitWidth: badge.implicitWidth+18; implicitHeight: 24; radius: 12; color: "#34402f"
                                    Text { id: badge; anchors.centerIn: parent; text: libraryController.stateText; color: Theme.accent; font.pixelSize: 10 }
                                }
                            }
                            Item { Layout.fillHeight: true }
                            Text { objectName: "currentTitle"; Layout.fillWidth: true; text: libraryController.title; textFormat: Text.PlainText; color: Theme.text; font.pixelSize: window.width > 1040 ? 26 : 22; font.weight: Font.DemiBold; elide: Text.ElideRight }
                            Text { Layout.fillWidth: true; text: libraryController.artist; textFormat: Text.PlainText; color: "#9eac9a"; font.pixelSize: 12; elide: Text.ElideRight }
                            Item { Layout.fillHeight: true }
                            Text { visible: window.height > 700; Layout.fillWidth: true; text: libraryController.audioInfo; textFormat: Text.PlainText; color: "#879681"; font.pixelSize: 10; elide: Text.ElideRight }
                        }
                    }
                }
                RowLayout {
                    spacing: 12
                    Text { text: qsTr("全部音乐"); color: Theme.text; font.pixelSize: 16; font.weight: Font.DemiBold }
                    Text { text: libraryController.visibleCount === libraryController.trackCount ? qsTr("%1 首").arg(libraryController.trackCount) : qsTr("%1 / %2 首").arg(libraryController.visibleCount).arg(libraryController.trackCount); color: Theme.muted; font.pixelSize: 11 }
                    Item { Layout.fillWidth: true }
                    TextField {
                        id: search
                        objectName: "searchField"
                        Layout.preferredWidth: window.width > 1060 ? 272 : 217
                        Layout.preferredHeight: 38
                        placeholderText: qsTr("搜索曲目、艺术家或文件夹")
                        placeholderTextColor: "#6f777c"
                        color: Theme.text
                        font.pixelSize: 11
                        leftPadding: 36; rightPadding: 30
                        selectByMouse: true
                        text: libraryController.searchText
                        onTextEdited: libraryController.searchText = text
                        onAccepted: { if(libraryController.visibleCount>0) { tracks.currentIndex=0; libraryController.selectRow(0); tracks.forceActiveFocus() } }
                        Accessible.name: qsTr("搜索音乐")
                        background: Rectangle { radius: 8; color: "#1e2326"; border.color: search.activeFocus ? "#809b69" : "#2b3235" }
                        Icon { anchors.left: parent.left; anchors.leftMargin: 10; anchors.verticalCenter: parent.verticalCenter; width: 16; height: 16; name: "search"; color: Theme.muted }
                        ActionButton { anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter; width: 30; height: 30; compact: true; flat: true; iconName: "close"; tip: qsTr("清空搜索"); visible: search.text.length>0; onClicked: libraryController.searchText="" }
                    }
                    ActionButton { objectName: "removeButton"; compact: true; flat: true; iconName: "trash"; tip: qsTr("从列表移除（Delete）"); enabled: libraryController.selectionCount>0; onClicked: libraryController.removeSelected() }
                }
                ColumnLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 0
                    RowLayout {
                        visible: libraryController.visibleCount>0
                        Layout.fillWidth: true; Layout.preferredHeight: 32
                        Layout.leftMargin: 12; Layout.rightMargin: 16
                        spacing: 16
                        Text { Layout.preferredWidth: 28; text: "#"; color: "#606a70"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter }
                        Text { Layout.fillWidth: true; text: qsTr("曲目"); color: "#6f7a80"; font.pixelSize: 10 }
                        Text { Layout.preferredWidth: tracks.width>650 ? 170 : 115; text: qsTr("艺术家 / 文件夹"); color: "#6f7a80"; font.pixelSize: 10 }
                        Text { Layout.preferredWidth: 54; text: qsTr("格式"); color: "#6f7a80"; font.pixelSize: 10; horizontalAlignment: Text.AlignHCenter }
                    }
                    Rectangle { visible: libraryController.visibleCount>0; Layout.fillWidth: true; height: 1; color: Theme.border }
                    Item {
                        Layout.fillWidth: true; Layout.fillHeight: true
                        ListView {
                            id: tracks
                            objectName: "trackList"
                            anchors.fill: parent
                            anchors.topMargin: 6
                            clip: true
                            spacing: 3
                            model: libraryController.playlist
                            boundsBehavior: Flickable.StopAtBounds
                            reuseItems: true
                            currentIndex: -1
                            keyNavigationEnabled: true
                            highlightMoveDuration: 0
                            Accessible.name: qsTr("音乐列表")
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded; width: 6 }
                            delegate: TrackRow {
                                width: tracks.width
                                title: model.trackTitle
                                subtitle: model.artist.length>0 ? model.artist : model.folder
                                fileType: model.format; path: model.filePath
                                number: model.trackNumber; current: model.isCurrent; selected: model.isSelected
                                rowIndex: index
                                onSelectedRow: function(extend) { tracks.currentIndex=rowIndex; libraryController.selectRow(rowIndex,extend); tracks.forceActiveFocus() }
                                onActivated: libraryController.playRow(rowIndex)
                            }
                            Keys.onReturnPressed: if(currentIndex>=0) libraryController.playRow(currentIndex)
                            Keys.onEnterPressed: if(currentIndex>=0) libraryController.playRow(currentIndex)
                            Keys.onUpPressed: { currentIndex=Math.max(0,currentIndex-1); libraryController.selectRow(currentIndex) }
                            Keys.onDownPressed: { currentIndex=Math.min(count-1,currentIndex+1); libraryController.selectRow(currentIndex) }
                        }
                        ColumnLayout {
                            anchors.centerIn: parent
                            visible: libraryController.visibleCount===0
                            spacing: 13
                            Icon { Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 36; Layout.preferredHeight: 36; name: libraryController.trackCount===0 ? "music" : "search"; color: Theme.accent }
                            Text { Layout.alignment: Qt.AlignHCenter; text: libraryController.trackCount===0 ? qsTr("让音乐填满这里") : qsTr("没有找到匹配的音乐"); color: Theme.text; font.pixelSize: 18; font.weight: Font.DemiBold }
                            Text { Layout.alignment: Qt.AlignHCenter; text: libraryController.trackCount===0 ? qsTr("拖入音频或文件夹，也可以从本地添加") : qsTr("试试其他关键词，或清空搜索"); color: Theme.muted; font.pixelSize: 12 }
                            ActionButton { Layout.alignment: Qt.AlignHCenter; text: qsTr("添加第一首音乐"); iconName: "add"; visible: libraryController.trackCount===0; onClicked: libraryController.addFiles() }
                        }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Rectangle { width: 5; height: 5; radius: 3; color: libraryController.statusError ? "#f0a89e" : libraryController.importing ? Theme.accent : "#657364" }
                    Text { objectName: "statusMessage"; Layout.fillWidth: true; text: libraryController.statusMessage; textFormat: Text.PlainText; color: libraryController.statusError ? "#efb0a8" : "#748078"; font.pixelSize: 10; wrapMode: Text.Wrap; maximumLineCount: 3; elide: Text.ElideRight }
                    ActionButton { compact: true; flat: true; implicitWidth: 26; implicitHeight: 26; iconName: "close"; visible: libraryController.statusError; tip: qsTr("关闭提示"); onClicked: libraryController.dismissMessage() }
                }
            }
        }
        PlayerBar {
            id: playbackBar
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 114
        }
    }
    DropArea {
        id: dropArea
        anchors.fill: parent
        onEntered: function(drag) { drag.accepted = drag.hasUrls }
        onDropped: function(drop) { if(drop.hasUrls) { libraryController.importUrls(drop.urls); drop.acceptProposedAction() } }
        Rectangle {
            anchors.fill: parent; anchors.margins: 8
            visible: dropArea.containsDrag
            color: "#ee1d291e"; radius: 14; border.width: 2; border.color: Theme.accent
            Column {
                anchors.centerIn: parent; spacing: 18
                Icon { anchors.horizontalCenter: parent.horizontalCenter; name: "add"; color: Theme.accent; width: 42; height: 42 }
                Text { text: qsTr("松开鼠标，添加到音乐库"); color: Theme.text; font.pixelSize: 24 }
                Text { anchors.horizontalCenter: parent.horizontalCenter; text: qsTr("支持音频文件和文件夹"); color: Theme.muted; font.pixelSize: 13 }
            }
        }
    }
    Connections {
        target: libraryController
        function onScrollToRow(row) { tracks.currentIndex=row; tracks.positionViewAtIndex(row,ListView.Center) }
    }
    Dialog {
        id: helpDialog
        title: qsTr("关于声间")
        anchors.centerIn: parent
        width: Math.min(window.width-80,500)
        modal: true
        standardButtons: Dialog.Ok
        contentItem: ColumnLayout {
            spacing: 16
            Text { text: qsTr("给音乐，一点自己的空间。"); color: Theme.text; font.pixelSize: 20; font.weight: Font.DemiBold }
            Text { Layout.fillWidth: true; text: qsTr("声间 2.0 · 本地音乐播放器\n双击曲目播放，Ctrl + 单击可多选。搜索仅筛选显示，上一首和下一首按完整音乐库切换。"); textFormat: Text.PlainText; color: Theme.muted; font.pixelSize: 13; wrapMode: Text.Wrap }
            Text { text: qsTr("空格　播放 / 暂停\nCtrl + O　添加音乐\nCtrl + Shift + O　导入文件夹\nCtrl + F　搜索\nCtrl + ← / →　上一首 / 下一首\nM　静音 / 恢复音量\nDelete　从列表移除，保留原文件"); color: Theme.text; font.pixelSize: 13; lineHeight: 1.5 }
            Text { Layout.fillWidth: true; text: qsTr("播放模式：顺序播放 / 列表循环 / 单曲循环 / 随机播放。音频输出使用系统默认设备。"); color: Theme.muted; font.pixelSize: 12; wrapMode: Text.Wrap }
        }
    }
}
