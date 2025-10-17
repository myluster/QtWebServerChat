import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

ListView {
    id: conversationView
    clip: true // 裁剪超出边界的内容
    spacing: 0 // 列表项之间不留空隙
    // 启用缓存以提高性能
    cacheBuffer: 100
    // 存储当前选中的会话 ID
    property string currentConversationId: ""

    // 使用临时的 ListModel 作为数据源
    model: ListModel {
        ListElement { conversationId: "user_001"; name: "文件传输助手"; lastMessage: "图片已收到";}
        ListElement { conversationId: "group_001"; name: "技术交流群"; lastMessage: "张三：新版本已发布111111111111111111111111111111111！";}
    }

    // 定义每个列表项的外观和行为
    delegate: Rectangle {
        width: conversationView.width
        height: Theme.listHeight
        // 根据当前项是否被选中来设置背景颜色
        color: model.conversationId === conversationView.currentConversationId ? Theme.choiceListColor : "transparent"


        ColumnLayout {
            // 水平方向上填充父级，并留出边距
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: Theme.spacingNormal
            anchors.rightMargin: Theme.spacingNormal
            // 垂直方向上居中
            anchors.verticalCenter: parent.verticalCenter
            spacing: 4

            Label {
                text: model.name
                font.pixelSize: Theme.fontSizeNormal
                color: Theme.primaryTextColor
                Layout.preferredWidth: parent.width * 0.95
                elide: Text.ElideRight
                wrapMode: Text.NoWrap // 防止换行
            }
            Label {
                text: model.lastMessage
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.secondaryTextColor
                elide: Text.ElideRight // 如果消息太长，用省略号结尾
                Layout.preferredWidth: parent.width * 0.95
                wrapMode: Text.NoWrap // 防止换行
            }
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                // 如果点击的是已选中的项，则取消选中
                if (conversationView.currentConversationId === model.conversationId) {
                    conversationView.currentConversationId = ""
                } else {
                    conversationView.currentConversationId = model.conversationId
                }
            }
        }
    }
}
