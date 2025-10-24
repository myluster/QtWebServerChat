import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import ".."
import Network 1.0
import Logger 1.0

Page {
    id: loginPage
    background: Rectangle {
        color: "transparent"
        radius: Theme.cornerRadius
    }

    Connections {
        target: NetworkManager

        function onConnectionStateChanged(connected) {
            Logger.info("Connection state changed: " + connected);
            if (!connected) {
                errorLabel.text = "与服务器断开连接"
                errorLabel.visible = true
            } else {
                errorLabel.visible = false
            }
        }

        function onLoginResponseReceived(success, message) {
            Logger.info("Login response received - Success: " + success + " Message: " + message);
            Logger.info("Attempting to enable login button");
            loginButton.enabled = true;

            if (success) {
                Logger.info("Login successful, attempting to navigate to main page");
                Logger.info("Checking if Signals object exists: " + (typeof Signals !== 'undefined'));
                Logger.info("Checking if goToMainPage function exists: " + (typeof Signals.goToMainPage === 'function'));
                
                // 确保rootStack存在并可访问
                Logger.info("Checking if rootStack exists: " + (typeof rootStack !== 'undefined'));
                
                try {
                    if (typeof Signals !== 'undefined' && typeof Signals.goToMainPage === 'function') {
                        Logger.info("Calling Signals.goToMainPage()");
                        Signals.goToMainPage();
                        Logger.info("After calling Signals.goToMainPage()");
                    } else {
                        Logger.error("Signals object or goToMainPage function not found!");
                        errorLabel.text = "页面导航错误，请检查应用程序配置";
                        errorLabel.visible = true;
                    }
                } catch (e) {
                    Logger.error("Exception occurred while navigating to main page: " + e);
                    errorLabel.text = "页面导航异常：" + e.toString();
                    errorLabel.visible = true;
                }
            } else {
                Logger.info("Login failed, showing error message");
                errorLabel.text = message;
                errorLabel.visible = true;
            }
        }

        function onErrorOccurred(error) {
            Logger.info("Network error occurred: " + error);
            loginButton.enabled = true
            errorLabel.text = "网络错误: " + error
            errorLabel.visible = true
        }
    }

    Component.onCompleted: {
        if (normalWidth !== 400 || normalHeight !== 600) {
            normalWidth = 400
            normalHeight = 600
        }
    }

    ColumnLayout {
        anchors.fill: parent
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        ColumnLayout {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            Layout.leftMargin: 40
            Layout.rightMargin: 40
            Layout.maximumWidth: 400

            spacing: Theme.spacingNormal

            Image {
                source: "qrc:/qml/assets/Weixin_PC_App_Icon.png"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 80
                fillMode: Image.PreserveAspectFit
                Layout.alignment: Qt.AlignHCenter
            }

            Label {
                text: "Chat"
                font.pixelSize: Theme.fontSizeTitle
                font.bold: true
                color: Theme.primaryTextColor
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: Theme.spacingLarge
            }

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

            Label {
                id: errorLabel
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                color: "red"
                font.pixelSize: Theme.fontSizeSmall
                visible: false
            }

            Button {
                id: loginButton
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                text: "登录"
                onClicked: {
                    if (usernameField.text.trim() === "" || passwordField.text.trim() === "") {
                        errorLabel.text = "用户名和密码不能为空"
                        errorLabel.visible = true
                        return
                    }
                    errorLabel.visible = false
                    enabled = false

                    Logger.info("Sending login request with username: " + usernameField.text);
                    NetworkManager.connectToServer("http://127.0.0.1:8080")
                    NetworkManager.sendLoginRequest(usernameField.text, passwordField.text)
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
                id: registerButton
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                text: "注册新用户"
                flat: true

                onClicked: {
                    rootStack.push("qrc:/qml/views/RegisterPage.qml")
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
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Theme.spacingNormal
            Layout.rightMargin: 40
            spacing: Theme.spacingNormal / 2 // 文本和方框间的小间距

            Label {
                id: proxyLabel
                text: "使用系统代理"
                font.pixelSize: Theme.fontSizeSmall
                color: Theme.primaryTextColor
                Layout.alignment: Qt.AlignVCenter // 垂直居中
            }

            CheckBox {
                id: proxyCheckBox
                Layout.alignment: Qt.AlignVCenter

                checked: NetworkManager.useProxy

                // 当勾选时，更新全局 Signals.useProxy
                onCheckedChanged: {
                    NetworkManager.useProxy = checked
                }
            }

            Item { Layout.fillWidth: true } // 将 CheckBox 推到左侧
        }
    }
}
