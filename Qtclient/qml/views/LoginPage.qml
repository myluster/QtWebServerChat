import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import ".."
import Network 1.0

Page {
    id: loginPage
    background: Rectangle {
        color: "transparent"
        radius: Theme.cornerRadius
    }

    Connections {
        target: NetworkManager

        function onConnectionStateChanged(connected) {
            console.log("Connection state changed:", connected);
            if (!connected) {
                errorLabel.text = "与服务器断开连接"
                errorLabel.visible = true
            } else {
                errorLabel.visible = false
            }
        }

        function onLoginResponseReceived(success, message) {
            console.log("Login response received - Success:", success, "Message:", message);
            console.log("Attempting to enable login button");
            loginButton.enabled = true;

            if (success) {
                console.log("Login successful, attempting to navigate to main page");
                console.log("Checking if Signals object exists:", typeof Signals !== 'undefined');
                console.log("Checking if goToMainPage function exists:", typeof Signals.goToMainPage === 'function');
                
                // 确保rootStack存在并可访问
                console.log("Checking if rootStack exists:", typeof rootStack !== 'undefined');
                
                try {
                    if (typeof Signals !== 'undefined' && typeof Signals.goToMainPage === 'function') {
                        console.log("Calling Signals.goToMainPage()");
                        Signals.goToMainPage();
                        console.log("After calling Signals.goToMainPage()");
                    } else {
                        console.error("Signals object or goToMainPage function not found!");
                        errorLabel.text = "页面导航错误，请检查应用程序配置";
                        errorLabel.visible = true;
                    }
                } catch (e) {
                    console.error("Exception occurred while navigating to main page:", e);
                    errorLabel.text = "页面导航异常：" + e.toString();
                    errorLabel.visible = true;
                }
            } else {
                console.log("Login failed, showing error message");
                errorLabel.text = message;
                errorLabel.visible = true;
            }
        }

        function onErrorOccurred(error) {
            console.log("Network error occurred:", error);
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

                    console.log("Sending login request with username:", usernameField.text);
                    NetworkManager.connectToServer("http://localhost:8080")
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
    }
}