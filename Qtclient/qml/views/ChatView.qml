import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

Item {
    id: chatView

    property string chatName: "Chat"
    property string chatId: ""

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

                // ScrollView 或 ListView 可以放在这里
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
                            console.log("发送消息: " + messageInput.text)
                            messageInput.text = ""
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
                    }
                }
            }
        }
    }
}
