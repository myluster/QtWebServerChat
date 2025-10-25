import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import ".."
import Network 1.0
import Logger 1.0

Item {
    id: mainPage

    //集中管理当前选中的ID
    property string currentConversationId: ""
    property string currentContactId: ""

    property string currentUserId: NetworkManager.currentUserId
        property string currentUsername: NetworkManager.currentUsername

    // 暴露子组件的引用，以便其他组件可以访问
    property alias chatButton: chatButton
    property alias contactsButton: contactsButton
    property alias middlePaneStack: middlePaneStack
    property alias conversationView: conversationView
    property alias contactView: contactView
    property alias rightPaneLoader: rightPaneLoader

    Connections {
        target: NetworkManager
        
        function onMessageReceived(message) {
            // 处理接收到的消息
            handleMessageReceived(message)
        }
    }
    
    // 添加处理消息的函数
    function handleMessageReceived(message) {
        if (message.type === "text_message") {
            // 将消息传递给当前的聊天视图
            if (typeof rightPaneLoader.item !== "undefined" &&
                rightPaneLoader.item !== null &&
                // 确保 ChatView 已经加载
                rightPaneLoader.item.hasOwnProperty("addMessage") &&
                // 确保消息是发给当前打开的聊天的
                // 注意：服务器发来的消息中应包含 "sender_id"
                rightPaneLoader.item.chatId === message.sender_id) {
                // 构造 ChatView.qml 中 addMessage 函数需要的对象
                    var formattedMessage = {
                        "isSent": false,
                        "content": message.content, // addMessage 需要 .content
                        "timestamp": message.timestamp || "??:??", // 确保服务端发送了时间戳
                        "sender_name": message.sender_name || "???" // 确保服务端发送了发送者名字
                    };

                    rightPaneLoader.item.addMessage(formattedMessage)
            } else {
                Logger.info("收到消息，但聊天窗口未打开或不匹配: " + message.sender_id)
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧侧边栏 (固定宽度，不在SplitView中)
        Item {
            id: leftSidebar
            Layout.preferredWidth: 60
            Layout.fillHeight: true
            clip: true

            // 添加背景色
            Rectangle {
                height: parent.height
                radius: Theme.cornerRadius
                //让矩形比父容器宽，从而将右侧圆角"推"出去
                width: parent.width + radius
                color: Theme.sidebarBackgroundColor
                z: -1 // 确保背景在按钮后面
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 20
                spacing: Theme.spacingLarge // 使用主题间距

                //按钮组，确保只有一个按钮可以被选中
                ButtonGroup{
                    id: sidebarButtonGroup
                }

                Button {
                    id: chatButton
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    checkable: true

                    //加入按钮组
                    ButtonGroup.group: sidebarButtonGroup

                    //默认选中这个按钮
                    checked: true

                    onClicked: {
                        //设置中面板显示会话窗口（索引为0）
                        middlePaneStack.currentIndex = 0
                        //根据记忆内容刷新右侧窗口
                        refreshRightPane()
                    }

                    contentItem: Image {
                        width: 24
                        height: 24
                        anchors.centerIn: parent

                        source: parent.checked ? "qrc:/qml/assets/chat-3-fill.svg" : "qrc:/qml/assets/chat-3-line.svg"

                        // 保持 SVG 的矢量特性
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        antialiasing: true
                        mipmap: true
                    }

                    background: Rectangle {
                        // 平时无颜色，悬停时使用专门的悬停颜色
                        color: parent.hovered ? Theme.buttonHoverColor : "transparent"
                        radius: Theme.cornerRadius
                    }
                }

                Button {
                    id: contactsButton
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    checkable: true

                    ButtonGroup.group: sidebarButtonGroup

                    onClicked: {
                        middlePaneStack.currentIndex = 1
                        refreshRightPane()
                    }

                    contentItem: Image {
                        width: 24
                        height: 24
                        anchors.centerIn: parent  // 添加居中对齐

                        source: parent.checked ? "qrc:/qml/assets/contacts-fill.svg" : "qrc:/qml/assets/contacts-line.svg"

                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        antialiasing: true
                        mipmap: true
                    }

                    background: Rectangle {
                        // 平时无颜色，悬停时使用专门的悬停颜色
                        color: parent.hovered ? Theme.buttonHoverColor : "transparent"
                        radius: Theme.cornerRadius
                    }
                }

                Item { Layout.fillHeight: true } // 占位符，将退出按钮推到底部

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48

                    onClicked: {
                        // 主动关闭WebSocket连接
                        NetworkManager.disconnectFromServer()
                        Signals.backToLoginPage() // 返回登录页
                    }

                    // 应用主题样式，使用图标替代文字
                    contentItem: Image {
                        source: "qrc:/qml/assets/logout-box-line.svg"
                        fillMode: Image.PreserveAspectFit
                        horizontalAlignment: Image.AlignHCenter
                        verticalAlignment: Image.AlignVCenter
                        sourceSize.width: 24
                        sourceSize.height: 24
                        smooth: true
                        antialiasing: true
                        mipmap: true
                    }

                    background: Rectangle {
                        // 平时无颜色，悬停时使用专门的悬停颜色
                        color: parent.hovered ? Theme.buttonHoverColor : "transparent"
                        radius: Theme.cornerRadius
                    }
                }
            }

            // 添加右侧分割线
            Rectangle {
                width: 1
                height: parent.height
                anchors.right: parent.right
                color: Theme.dividerColor
                z: -1 // 确保分割线在按钮后面
            }
        }

        // 右侧区域使用SplitView处理中间面板和右侧内容区
        SplitView {
            id: rightSplitView
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            handle: Rectangle {
                implicitWidth: 1      // 使用 implicitWidth 来建议宽度
                color: Theme.dividerColor
            }

            // 中侧列表面板
            Item {
                id: middlePane
                SplitView.preferredWidth: 280
                SplitView.minimumWidth: 220

                // 添加背景色
                Rectangle {
                    anchors.fill: parent
                    color: Theme.listBackgroundColor
                }

                StackLayout {
                    id: middlePaneStack
                    anchors.fill: parent
                    currentIndex: 0 // 默认显示会话列表

                    //会话列表视图
                    ConversationListView {
                        id: conversationView
                        // 修复：正确的属性绑定方式
                        onCurrentConversationIdChanged: {
                            mainPage.currentConversationId = conversationView.currentConversationId
                            refreshRightPane()
                        }
                    }

                    //联系人列表视图
                    ContactListView {
                        id: contactView
                        // 修复：正确的属性绑定方式
                        onCurrentContactIdChanged: {
                            mainPage.currentContactId = contactView.currentContactId
                            refreshRightPane()
                        }
                    }
                }
            }

            // 右侧主内容区
            Item {
                id: rightPane
                SplitView.minimumWidth: 400
                SplitView.fillWidth: true // 关键：填充剩余空间
                clip: true

                // 添加背景色
                Rectangle {
                    radius: Theme.cornerRadius
                    height: parent.height
                    x: -radius
                    width: parent.width + radius
                    color: Theme.chatBackgroundColor
                }

                Loader {
                    id: rightPaneLoader
                    anchors.fill: parent
                }
            }
        }
    }

    // 刷新右侧窗口的函数
    function refreshRightPane() {
        if (chatButton.checked) {
            // 如果聊天按钮被选中
            if (currentConversationId !== "") {
                // 如果有选中的会话，则加载对应的聊天视图
                var currentConversation = getConversationById(currentConversationId)
                if (currentConversation) {
                    Logger.info("Loading ContactProfileView with mainPage reference");  // 添加调试信息
                    rightPaneLoader.setSource("qrc:/qml/views/ChatView.qml",
                                              { "chatName": currentConversation.name,
                                                "chatId": currentConversation.conversationId })
                }
            } else {
                // 否则清除右侧窗口内容
                rightPaneLoader.source = ""
            }
        } else if (contactsButton.checked) {
            // 如果联系人按钮被选中
            if (currentContactId !== "") {
                // 如果有选中的联系人，则加载对应的联系人简介视图
                var currentContact = getContactById(currentContactId)
                if (currentContact) {
                    Logger.info("Loading ContactProfileView with mainPage reference");  // 添加调试信息
                    rightPaneLoader.setSource("qrc:/qml/views/ContactProfileView.qml",
                                              { "contactName": currentContact.name,
                                                "contactId": currentContact.contactId,
                                                "contactStatus": currentContact.status,
                                                "mainPage": mainPage })
                }
            } else {
                // 否则清除右侧窗口内容
                rightPaneLoader.source = ""
            }
        }
    }

    // 根据会话ID获取会话信息的辅助函数
    function getConversationById(conversationId) {
        for (var i = 0; i < conversationView.model.count; i++) {
            var conversation = conversationView.model.get(i)
            if (conversation.conversationId === conversationId) {
                return conversation
            }
        }
        return null
    }

    // 根据联系人ID获取联系人信息的辅助函数
    function getContactById(contactId) {
        for (var i = 0; i < contactView.model.count; i++) {
            var contact = contactView.model.get(i)
            if (contact.contactId === contactId) {
                return contact
            }
        }
        return null
    }

    // 删除联系人的函数
    function removeContact(contactId) {
        // 遍历联系人列表查找要删除的联系人
        for (var i = 0; i < contactView.model.count; i++) {
            var contact = contactView.model.get(i)
            if (contact.contactId === contactId) {
                // 从联系人列表中删除该联系人
                contactView.model.remove(i)
                break
            }
        }

        // 如果当前选中的联系人就是被删除的联系人，则清除选中状态
        if (currentContactId === contactId) {
            currentContactId = ""
            refreshRightPane()
        }

        // 注意：会话窗口不会被删除，这样用户仍然可以查看与该联系人的历史消息
    }
}
