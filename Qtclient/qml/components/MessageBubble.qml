import QtQuick
import QtQuick.Controls
import "../theme"

Item {
    id: bubbleRoot

    // 属性定义
    property bool isSent: false           // 是否是发送的消息
    property string messageText: ""       // 消息文本
    property string timestamp: ""         // 时间戳
    property string senderName: ""        // 发送者名称（接收的消息需要）
    property int maxWidth: 400            // 气泡最大宽度

    // 计算属性
    property color bubbleColor: isSent ? Theme.outgoingBubbleColor : Theme.incomingBubbleColor
    property color textColor: isSent ? Theme.primaryTextColor : Theme.primaryTextColor
    property int bubblePadding: Theme.spacingNormal

    width: parent.width
    height: columnLayout.height + 2 * bubblePadding

    Column {
        id: columnLayout
        anchors.right: isSent ? parent.right : undefined
        anchors.left: isSent ? undefined : parent.left
        anchors.margins: Theme.spacingNormal
        spacing: 2

        // 发送者名称（仅对接收的消息显示）
        Label {
            visible: !isSent && senderName.length > 0
            text: senderName
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryTextColor
            anchors.left: parent.left
            anchors.leftMargin: bubbleRoot.isSent ? undefined : bubbleRoot.bubblePadding
            // 启用缓存以提高性能
            layer.enabled: true
        }

        // 消息气泡
        Rectangle {
            id: bubble
            width: Math.min(messageLabel.implicitWidth + 2 * bubblePadding, maxWidth - 2 * bubblePadding)
            height: messageLabel.implicitHeight + 2 * bubblePadding
            color: bubbleColor
            radius: Theme.cornerRadius
            border.color: isSent ? "transparent" : "#E0E0E0"
            border.width: 1
            anchors.right: isSent ? parent.right : undefined
            anchors.left: isSent ? undefined : parent.left
            // 启用缓存以提高性能
            layer.enabled: true

            // 消息文本
            Label {
                id: messageLabel
                anchors.centerIn: parent
                text: messageText
                font.pixelSize: Theme.fontSizeNormal
                color: textColor
                wrapMode: Text.Wrap
                maximumLineCount: 100
                // 启用缓存以提高性能
                layer.enabled: true
            }
        }

        // 时间戳
        Label {
            text: timestamp
            font.pixelSize: Theme.fontSizeSmall
            color: Theme.secondaryTextColor
            anchors.right: isSent ? parent.right : undefined
            anchors.left: isSent ? undefined : parent.left
            anchors.leftMargin: bubbleRoot.isSent ? undefined : bubbleRoot.bubblePadding
            // 启用缓存以提高性能
            layer.enabled: true
        }
    }
}
