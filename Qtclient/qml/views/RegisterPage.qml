import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import Network 1.0

Page {
    id: registerPage
    background: Rectangle {
        color: "transparent"
        radius: Theme.cornerRadius
    } // 使用主题背景色

    Connections {
        target: NetworkManager

        function onRegisterResponseReceived(success, message) {
            registerButton.enabled = true
            if (success) {
                errorLabel.text = message
                errorLabel.color = "green"
                errorLabel.visible = true
                // 注册成功后，返回登录页面
                rootStack.pop()
            } else {
                errorLabel.text = message
                errorLabel.color = "red"
                errorLabel.visible = true
            }
        }

        function onErrorOccurred(error) {
            registerButton.enabled = true
            errorLabel.text = error
            errorLabel.color = "red"
            errorLabel.visible = true
        }
    }

    // 最外层的布局，用于实现自适应和垂直居中
    ColumnLayout {
        anchors.fill: parent

        // 弹簧1: 占据顶部的多余垂直空间
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        // 内容容器: 所有 UI 控件都放在这里
        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter // 在父布局中水平居中
            Layout.fillWidth: true
            Layout.leftMargin: 40
            Layout.rightMargin: 40
            Layout.maximumWidth: 400 // 设置最大宽度，防止在宽屏上过度拉伸
            spacing: Theme.spacingNormal // 内部控件的间距

            // --- 页面标题 ---
            Label {
                text: "创建新账户"
                font.pixelSize: Theme.fontSizeTitle+10
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
                    placeholderText: "设置用户名"
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
                    placeholderText: "设置密码"
                    echoMode: TextInput.Password
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
                    id: confirmPasswordField
                    anchors.fill: parent
                    anchors.leftMargin: Theme.spacingNormal
                    anchors.rightMargin: Theme.spacingNormal
                    placeholderText: "确认密码"
                    echoMode: TextInput.Password
                    color: Theme.primaryTextColor
                    font.pixelSize: Theme.fontSizeNormal
                    background: Rectangle { color: "transparent" }
                    selectByMouse: true
                    Keys.onReturnPressed: registerButton.clicked()
                }
            }

            // --- 错误提示标签 ---
            Label {
                id: errorLabel
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: "red"
                font.pixelSize: Theme.fontSizeSmall
                visible: false
            }

            // --- 按钮 ---
            Button {
                id: registerButton
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                text: "注册"
                onClicked: {
                    // 简单的前端验证
                    if (usernameField.text.trim() === "") {
                        errorLabel.text = "用户名不能为空"
                        errorLabel.visible = true
                        return
                    }
                    
                    if (passwordField.text.trim() === "") {
                        errorLabel.text = "密码不能为空"
                        errorLabel.visible = true
                        return
                    }
                    
                    if (passwordField.text !== confirmPasswordField.text) {
                        errorLabel.text = "两次输入的密码不一致"
                        errorLabel.visible = true
                        return
                    }
                    
                    errorLabel.visible = false
                    registerButton.enabled = false
                    
                    // 发送注册请求
                    NetworkManager.connectToServer("http://localhost:8080")
                    NetworkManager.sendRegisterRequest(usernameField.text, passwordField.text)
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
                    color: parent.down? Qt.darker(Theme.primaryColor, 1.2) : Theme.primaryColor
                    radius: Theme.cornerRadius
                }
            }

            Button {
                id: backButton
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                text: "返回登录"
                onClicked: {
                    // 从导航栈中弹出当前页面，返回上一页
                    rootStack.pop()
                }

                contentItem: Label {
                    text: parent.text
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeNormal
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    color: "transparent"
                }
            }
        } // --- 内容容器结束 ---

        // 弹簧2: 与顶部的弹簧一起平分剩余空间，实现完美的垂直居中
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }
    }
}