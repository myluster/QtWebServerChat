import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import Logger 1.0

Item {
    id: contactProfileView

    // 接收从联系人列表传递过来的属性
    property string contactName: ""
    property string contactId: ""
    property string contactStatus: ""

    // 添加对MainPage的引用
    property var mainPage: null

    // 使用锚点布局使内部所有内容在视图中居中
    ColumnLayout {
        anchors.centerIn: parent
        width: parent.width * 0.8 // 限制一下宽度，避免在大屏上过于分散
        spacing: Theme.spacingLarge

        // 基本信息区域
        Label {
            text: contactName
            font.pixelSize: Theme.fontSizeLarge
            font.bold: true
            color: Theme.primaryTextColor
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "ID: " + contactId
            font.pixelSize: Theme.fontSizeNormal
            color: Theme.secondaryTextColor
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            text: "状态: " + contactStatus
            font.pixelSize: Theme.fontSizeNormal
            color: contactStatus === "在线" ? Theme.onlineStatusColor : Theme.offlineStatusColor
            Layout.alignment: Qt.AlignHCenter
        }

        // 在信息和按钮之间增加一些间距
        Item {
            Layout.preferredHeight: Theme.spacingLarge
        }

        // 操作按钮行
        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spacingNormal // 两个按钮之间的间距

            // 发送消息按钮
            Button {
                Layout.fillWidth: true // 均分行宽度
                Layout.preferredHeight: 44
                text: "发送消息"
                onClicked: {
                    Logger.info("发送消息给: " + contactName);
                    // 跳转到会话模式并打开对应会话
                    sendToConversation(contactId, contactName)
                }

                contentItem: Label {
                    text: parent.text
                    color: "white"
                    font.bold: true
                    font.pixelSize: Theme.fontSizeLarge
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: parent.down ? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor
                    radius: Theme.cornerRadius
                }
            }

            // 删除好友按钮
            Button {
                Layout.fillWidth: true // 均分行宽度
                Layout.preferredHeight: 44
                text: "删除好友"
                onClicked: {
                    Logger.info("删除好友: " + contactName);
                    // 实现删除好友的逻辑
                    removeContact()
                }

                contentItem: Label {
                    text: parent.text
                    color: "white"
                    font.bold: true
                    font.pixelSize: Theme.fontSizeLarge
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    // 使用红色系作为背景，以警示用户这是一个危险操作
                    color: parent.down ? Qt.darker("#d32f2f", 1.2) : "#d32f2f"
                    radius: Theme.cornerRadius
                }
            }
        }
    }

    // 发送消息到会话的函数
    function sendToConversation(contactId, contactName) {
        // 检查mainPage是否已设置
        if (!mainPage) {
            Logger.error("MainPage reference is not set in ContactProfileView");
            return
        }

        // 切换到聊天视图
        mainPage.chatButton.checked = true
        mainPage.middlePaneStack.currentIndex = 0

        // 检查会话是否已存在
        var conversationExists = false
        var conversationIndex = -1

        // 遍历会话列表查找是否已存在该联系人的会话
        for (var i = 0; i < mainPage.conversationView.model.count; i++) {
            var conversation = mainPage.conversationView.model.get(i)
            if (conversation.conversationId === contactId) {
                conversationExists = true
                conversationIndex = i
                break
            }
        }

        if (conversationExists) {
            // 如果会话已存在，将其移动到列表顶部
            var existingConversation = mainPage.conversationView.model.get(conversationIndex)
            mainPage.conversationView.model.remove(conversationIndex)
            mainPage.conversationView.model.insert(0, existingConversation)

            // 选中该会话
            mainPage.conversationView.currentConversationId = contactId
        } else {
            // 如果会话不存在，创建新会话并添加到列表顶部
            mainPage.conversationView.model.insert(0, {
                "conversationId": contactId,
                "name": contactName,
                "lastMessage": "[新消息]"
            })

            // 选中新创建的会话
            mainPage.conversationView.currentConversationId = contactId
        }

        // 刷新右侧窗口
        mainPage.refreshRightPane()
    }

    // 删除联系人的函数
    function removeContact() {
        // 检查mainPage是否已设置
        if (!mainPage) {
            Logger.error("MainPage reference is not set in ContactProfileView");
            return
        }

        // 直接调用MainPage的删除联系人函数
        mainPage.removeContact(contactId)
    }
}