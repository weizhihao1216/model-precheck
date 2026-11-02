import QtQuick 2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

// 统一的等候提示层：一键预检、代码编译、单项测试等所有需要等待的操作共用。
Popup {
    id: overlay

    property bool active: false
    property string title: ""
    property string detail: ""
    property string hint: ""
    property string logLine: ""
    property int elapsedSeconds: 0
    // 等候进度：0.0~1.0 时在提示行显示完成百分比，负值不显示；绿条统一为往返扫动动画。
    property double progress: -1

    parent: Overlay.overlay
    x: Math.round((parent.width - width) / 2)
    y: Math.round((parent.height - height) / 2)
    width: Math.min(520, parent ? parent.width - 80 : 440)
    height: 280
    padding: 0
    modal: true
    focus: true
    closePolicy: Popup.NoAutoClose
    visible: active

    onActiveChanged: if (active) elapsedSeconds = 0

    Timer {
        running: overlay.active
        interval: 1000
        repeat: true
        onTriggered: overlay.elapsedSeconds += 1
    }

    background: Rectangle {
        radius: 12
        color: "white"
        border.color: "#dce4ec"
    }

    contentItem: ColumnLayout {
        anchors.margins: 28
        spacing: 12

        Item { Layout.fillHeight: true }

        BusyIndicator {
            Layout.alignment: Qt.AlignHCenter
            running: overlay.active
            implicitWidth: 58
            implicitHeight: 58
        }
        Text {
            Layout.fillWidth: true
            text: overlay.title
            color: "#14213d"
            font.pixelSize: 21
            font.weight: Font.Bold
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            Layout.fillWidth: true
            text: overlay.detail
            color: "#60708a"
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }
        Rectangle {
            id: progressTrack
            Layout.fillWidth: true
            Layout.leftMargin: 26
            Layout.rightMargin: 26
            height: 6
            implicitHeight: 6
            Layout.preferredHeight: 6
            radius: 3
            color: "#dcece9"

            Rectangle {
                id: progressThumb
                width: progressTrack.width * 0.32
                height: parent.height
                radius: parent.radius
                color: "#0d9488"

                // 用 0~1 的相位驱动往返位移，避免依赖首帧宽度（首帧宽度为 0 时动画终点恒为 0，绿条不动）。
                property real sweep: 0
                x: Math.max(0, (progressTrack.width - width) * sweep)
                SequentialAnimation on sweep {
                    running: overlay.active
                    loops: Animation.Infinite
                    NumberAnimation { from: 0; to: 1; duration: 1100; easing.type: Easing.InOutQuad }
                    NumberAnimation { from: 1; to: 0; duration: 1100; easing.type: Easing.InOutQuad }
                }
            }
        }
        Text {
            Layout.fillWidth: true
            visible: overlay.hint.length > 0
            text: (overlay.progress >= 0.0 && overlay.progress <= 1.0)
                  ? (overlay.hint + " (" + Math.round(overlay.progress * 100) + "%)")
                  : overlay.hint
            color: "#0d9488"
            font.pixelSize: 12
            font.weight: Font.DemiBold
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            Layout.fillWidth: true
            visible: overlay.logLine.length > 0
            text: overlay.logLine
            color: "#8898aa"
            font.pixelSize: 11
            font.family: "Consolas"
            elide: Text.ElideRight
            horizontalAlignment: Text.AlignHCenter
        }
        Text {
            Layout.fillWidth: true
            text: qsTr("已耗时 %1 秒").arg(overlay.elapsedSeconds)
            color: "#8898aa"
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
        }

        Item { Layout.fillHeight: true }
    }
}
