import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import "../components"

Item {
    id: chatView

    property string chatName: "Chat"
    property string chatId: ""

    // 聊天记录数据模型
    property var chatMessages: []

    ColumnLayout {
        anchors.fill: parent

        // 顶部标题栏
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: Theme.listHeight
            color: Theme.backgroundColor // 与主背景色一致

            Label {
                anchors.centerIn: parent
                text: chatView.chatName // 显示传入的聊天名称
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                color: Theme.primaryTextColor
            }

            Rectangle {
                height: 1
                width: parent.width
                anchors.bottom: parent.bottom
                color: Theme.dividerColor
            }
        }

        // 使用SplitView实现可调整的上下区域
        SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Vertical

            handle: Rectangle {
               implicitHeight: 1
                color: Theme.dividerColor
            }

            // 消息显示区域
            Rectangle {
                id: messageArea
                SplitView.fillWidth: true
                SplitView.fillHeight: true
                SplitView.minimumHeight: 100
                color: Theme.chatBackgroundColor

                // 聊天记录显示区域
                ScrollView {
                    id: chatScrollView
                    anchors.fill: parent
                    clip: true

                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    ListView {
                        id: messagesList
                        anchors.fill: parent
                        anchors.margins: 0

                        model: chatMessages

                        delegate: MessageBubble {
                            width: messagesList.width
                            isSent: modelData.isSent
                            messageText: modelData.text
                            timestamp: modelData.timestamp
                            senderName: modelData.senderName
                            maxWidth: messagesList.width * 0.7
                        }

                        // 设置间距
                        spacing: Theme.spacingNormal

                        // 默认显示在最新消息处
                        Component.onCompleted: {
                            positionViewAtEnd()
                        }
                    }
                }
            }

            // 输入区域
            Item {
                id: inputAreaContainer
                SplitView.fillWidth: true
                SplitView.preferredHeight: 200
                SplitView.minimumHeight: 80
                SplitView.maximumHeight: 400

                // 背景色
                Rectangle {
                    anchors.fill: parent
                    color: Theme.backgroundColor
                }

                // 发送按钮固定在右下
                Button {
                    id: sendButton
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    anchors.margins: Theme.spacingNormal

                    width: 60
                    height: 30
                    text: "发送"

                    property bool hasContent: messageInput.text.trim().length > 0

                    contentItem: Text {
                        text: parent.text
                        color: sendButton.hasContent ? Theme.listBackgroundColor : "#929292"
                        font.pixelSize: Theme.fontSizeLarge
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: Theme.cornerRadius/2
                        color: sendButton.hasContent ? Theme.primaryColor : Theme.buttonCloseColor
                    }

                    onClicked: function() {
                        if (sendButton.hasContent) {
                            sendMessage()
                        }
                    }
                }

                // 带滚动条的输入框
                ScrollView {
                    id: scrollView
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.bottom: sendButton.top
                    anchors.margins: Theme.spacingNormal

                    background: Rectangle {
                        color: "transparent"
                        border.color: "transparent"
                    }
                    clip: true

                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    TextArea {
                        id: messageInput
                        width: scrollView.width

                        font.pixelSize: Theme.fontSizeNormal
                        color: Theme.primaryTextColor
                        wrapMode: TextArea.Wrap
                        selectByMouse: true

                        background: Rectangle {
                            color: "transparent"
                        }

                        // 处理键盘事件：回车发送，Shift+回车换行
                        Keys.onPressed: function(event) {
                            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                                if (event.modifiers & Qt.ShiftModifier) {
                                    // Shift+回车：插入换行符
                                    insert(messageInput.cursorPosition, "\n")
                                } else {
                                    // 回车：发送消息（仅当有内容时）
                                    if (messageInput.text.trim().length > 0) {
                                        sendMessage()
                                    }
                                    event.accepted = true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 发送消息的函数
    function sendMessage() {
        if (messageInput.text.trim().length > 0) {
            // 创建新消息对象
            var newMessage = {
                "isSent": true,
                "text": messageInput.text,
                "timestamp": getCurrentTime(),
                "senderName": ""
            };

            // 添加到聊天记录
            chatMessages.push(newMessage);

            // 使用异步更新模型以避免阻塞UI
            Qt.callLater(function() {
                // 更新模型
                chatMessages = chatMessages.slice(0); // 创建新数组以触发更新

                // 清空输入框
                messageInput.text = "";

                // 滚动到最新消息
                messagesList.positionViewAtEnd();
            });
        }
    }

    // 获取当前时间的函数
    function getCurrentTime() {
        var now = new Date();
        return Qt.formatTime(now, "hh:mm");
    }

    // 加载聊天记录的函数（示例）
    function loadChatHistory() {
        // 这里应该从服务器或本地数据库加载聊天记录
        // 示例数据
        var sampleMessages = [
            {
                "isSent": false,
                "text": "你好！",
                "timestamp": "10:30",
                "senderName": "张三"
            },
            {
                "isSent": true,
                "text": "你好，有什么可以帮助你的吗？",
                "timestamp": "10:31",
                "senderName": ""
            },
            {
                "isSent": false,
                "text": "我想了解一下你们的产品。",
                "timestamp": "10:32",
                "senderName": "张三"
            }
        ];

        chatMessages = sampleMessages;
    }

    // 组件完成时加载聊天记录
    Component.onCompleted: {
        loadChatHistory();
    }
}
