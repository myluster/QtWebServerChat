import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import ".."

Page {
    id: loginPage
    background: Rectangle {
        color: "transparent"
        radius: Theme.cornerRadius
    } // 使用主题背景色

    Component.onCompleted: {
        // 如果当前退出全屏的窗口大小不是登录页面的默认大小，则重置为默认大小
        if (normalWidth !== 400 || normalHeight !== 600) {
            normalWidth = 400
            normalHeight = 600
        }
    }

    // 最外层的布局，用于实现自适应布局
    ColumnLayout {
        anchors.fill: parent
        // 占据所有多余的垂直空间，把内容向下推
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // 内容容器
        // 我们将所有登录控件都放进这个容器，以实现统一管理
        ColumnLayout {
            // 在父布局中水平居中
            Layout.alignment: Qt.AlignHCenter
            // 宽度自适应父布局，但左右留有边距
            Layout.fillWidth: true
            Layout.leftMargin: 40
            Layout.rightMargin: 40
            //设置一个最大宽度，防止在超宽屏幕上拉伸得太难看
            Layout.maximumWidth: 400

            spacing: Theme.spacingNormal// 内部控件的间距

            // --- Logo 和标题 ---
            // Image被包含在布局中，它的尺寸是固定的，不会失控
            Image {
                source: "qrc:/qml/assets/Weixin_PC_App_Icon.png"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 80
                fillMode: Image.PreserveAspectFit
                Layout.alignment: Qt.AlignHCenter // 在当前布局中居中
            }

            Label {
                text: "Chat"
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                color: Theme.primaryTextColor
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: Theme.spacingLarge
            }

            // --- 输入框 ---
            Rectangle {
                Layout.fillWidth: true
                height: 44
                color: Theme.incomingBubbleColor
                radius: Theme.cornerRadius
                border.color: '#E0E0E0'

                TextField {
                    id: usernameField
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingNormal
                    anchors.rightMargin: Theme.spacingNormal
                    placeholderText: "用户名"
                    color: Theme.primaryTextColor
                    font.pixelSize: Theme.fontSizeNormal
                    background: Rectangle { color: "transparent" }
                    selectByMouse: true
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 44
                color: Theme.incomingBubbleColor
                radius: Theme.cornerRadius
                border.color: "#E0E0E0"

                TextField {
                    id: passwordField
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingNormal
                    anchors.rightMargin: Theme.spacingNormal
                    placeholderText: "密码"
                    echoMode: TextInput.Password
                    color: Theme.primaryTextColor
                    font.pixelSize: Theme.fontSizeNormal
                    background: Rectangle { color: "transparent" }
                    selectByMouse: true
                    Keys.onReturnPressed: loginButton.clicked()
                }
            }

            // --- 错误提示标签 ---
            Label {
                id: errorLabel
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: "red"
                font.pixelSize: Theme.fontSizeSmall
                visible: false // 默认隐藏
            }

            // --- 按钮 ---
            Button {
                id: loginButton
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                text: "登录"
                onClicked: {
                    // 简单的前端验证
                    if (usernameField.text.trim() === "" || passwordField.text.trim() === "") {
                        //errorLabel.text = "用户名和密码不能为空"
                        //errorLabel.visible = true
                        //return
                    }
                    errorLabel.visible = false
                    // 发送切换至主界面的信号
                    Signals.goToMainPage()
                }

                // 按钮文本显示样式
                contentItem: Label {
                    text: parent.text
                    color: "white"
                    font.bold: true
                    font.pixelSize: Theme.fontSizeLarge
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }



                // 自定义按钮背景
                background: Rectangle {
                    // 当按钮被按下时，颜色变深，提供视觉反馈
                    color: parent.down? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor
                    radius: Theme.cornerRadius
                }
            }


            Button {
                id: registerButton
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                text: "注册新用户"
                flat: true // 使其看起来像一个链接

                onClicked: {
                    // 执行前端导航，推入注册页面
                    rootStack.push("qrc:/qml/views/RegisterPage.qml")
                }

                // 次要按钮的样式风格
                contentItem: Label {
                    text: parent.text
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeNormal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: "transparent" // 透明背景
                }
            }
        } // --- 内容容器结束 ---

        // 这个 Item 会和顶部的弹簧一起平分剩余空间，实现完美的垂直居中
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true //填充所有可用高度
        }
    }
}
