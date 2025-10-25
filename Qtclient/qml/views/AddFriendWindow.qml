import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import "../theme"
import Network 1.0
import Logger 1.0

ApplicationWindow {
    id: addFriendWindow
    width: 400
    height: 700
    minimumWidth: 400
    minimumHeight: 700
    modality: Qt.ApplicationModal

    // 监听来自 NetworkManager 的响应
    Connections {
        target: NetworkManager

        onAddFriendResponseReceived: function(success, message) {
            Logger.info("Add friend response: " + success + ", msg: " + message);
            infoText.text = message;
            infoText.color = success ? "green" : "red";
            infoText.opacity = 1.0;
        }

        onSearchUserResponseReceived: function(results) {
            Logger.info("Search results received: " + results.length + " items");
            searchResultModel.clear();

            if (results.length === 0) {
                infoText.text = "未找到用户";
                infoText.color = "orange";
                infoText.opacity = 1.0;
                return;
            }

            if (results && typeof results.forEach === 'function') {
                results.forEach(function(user) {
                    searchResultModel.append({
                        "userId": user.userId,
                        "userName": user.userName,
                        "userStatus": user.userStatus || "未知"
                    });
                });
            }
        }
    }

    // 移除原生边框，并使窗口背景完全透明
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"

    // 背景和阴影层
    Rectangle {
        id: backgroundAndShadow
        anchors.fill: parent
        anchors.margins: addFriendWindow.visibility === ApplicationWindow.Maximized ? 0 : 10
        color: "white"
        radius: addFriendWindow.visibility === ApplicationWindow.Maximized ? 0 : Theme.cornerRadius
        border.color: "#E0E0E0"
        border.width: 1

        // 阴影效果
        layer.enabled: addFriendWindow.visibility !== ApplicationWindow.Maximized
        layer.smooth: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 0
            radius: Theme.cornerRadius
            samples: 2*Theme.cornerRadius+1
            color: "#80000000"
        }
    }

    // 内容区域
    Rectangle {
        id: contentArea
        anchors.fill: parent
        anchors.margins: addFriendWindow.visibility === ApplicationWindow.Maximized ? 0 : 10
        radius: addFriendWindow.visibility === ApplicationWindow.Maximized ? 0 : Theme.cornerRadius
        color: "transparent"
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingNormal
            spacing: Theme.spacingNormal

            // 标题栏
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                color: Theme.sidebarBackgroundColor

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.spacingNormal
                    spacing: Theme.spacingNormal

                    Item {
                        Layout.fillWidth: true
                    }

                    // 关闭按钮
                    Button {
                        Layout.preferredWidth: 30
                        Layout.preferredHeight: 30

                        contentItem: Image {
                            source: "qrc:/qml/assets/close-line.svg"
                            fillMode: Image.PreserveAspectFit
                            sourceSize.width: 16
                        }

                        background: Rectangle {
                            color: parent.hovered ? "#FF6B6B" : "transparent"
                            radius: Theme.cornerRadius
                        }

                        onClicked: {
                            addFriendWindow.close()
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onPressed: {
                        addFriendWindow.startSystemMove()
                    }
                }
            }

            // Tab控件
            TabBar {
                id: tabBar
                Layout.fillWidth: true
                currentIndex: 0

                TabButton {
                    text: "添加好友"
                }

                TabButton {
                    text: "好友申请"
                }
            }

            // Stack布局用于切换内容
            StackLayout {
                id: stackLayout
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: tabBar.currentIndex

                // 添加好友页面
                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingNormal
                        spacing: Theme.spacingNormal

                        // 搜索框
                        TextField {
                            id: searchField
                            Layout.fillWidth: true
                            placeholderText: "请输入好友ID或昵称"
                            selectByMouse: true
                        }

                        // 搜索按钮
                        Button {
                            Layout.fillWidth: true
                            text: "搜索"

                            // 将搜索按钮背景颜色改为标志绿色
                            background: Rectangle {
                                color: Theme.primaryColor
                                radius: Theme.cornerRadius
                            }

                            contentItem: Text {
                                text: "搜索"
                                color: "white"
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: Theme.fontSizeNormal
                            }

                            onClicked: {
                                if (searchField.text.trim() !== "") {
                                    searchResultModel.clear();
                                    Logger.info("Calling NetworkManager.searchUser with: " + searchField.text.trim());
                                    NetworkManager.searchUser(searchField.text.trim());
                                } else {
                                    infoText.text = "请输入搜索内容";
                                    infoText.color = "red";
                                    infoText.opacity = 1.0;
                                }
                            }
                        }

                        // 搜索结果列表
                        ListView {
                            id: searchResultList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip:true

                            model: ListModel {
                                id: searchResultModel
                            }

                            delegate: Rectangle {
                                width: searchResultList.width
                                height: Theme.listHeight
                                color: "transparent"

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingNormal
                                    spacing: Theme.spacingNormal

                                    // 移除用户头像（占位符）
                                    /*
                                    Rectangle {
                                        Layout.preferredWidth: 40
                                        Layout.preferredHeight: 40
                                        radius: 20
                                        color: Theme.onlineStatusColor
                                    }
                                    */

                                    ColumnLayout {
                                        Layout.fillWidth: true
                                        spacing: 4

                                        Label {
                                            text: model.userName
                                            font.pixelSize: Theme.fontSizeNormal
                                            color: Theme.primaryTextColor
                                            elide: Text.ElideRight
                                        }

                                        Label {
                                            text: "ID: " + model.userId + " (" + model.userStatus + ")"
                                            font.pixelSize: Theme.fontSizeSmall
                                            color: Theme.secondaryTextColor
                                            elide: Text.ElideRight
                                        }
                                    }

                                    Button {
                                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                                        text: "添加"
                                        onClicked: {
                                            if (mainPage && mainPage.currentUserId) {
                                                Logger.info("Sending add friend request from user: " + mainPage.currentUserId + " to user: " + model.userId);
                                                NetworkManager.sendAddFriendRequest(mainPage.currentUserId, model.userId);
                                            } else {
                                                Logger.error("无法添加好友: mainPage.currentUserId 未定义");
                                                infoText.text = "客户端错误：未登录";
                                                infoText.color = "red";
                                                infoText.opacity = 1.0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // 好友申请页面
                Item {
                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: Theme.spacingNormal
                        spacing: Theme.spacingNormal

                        // 申请列表
                        ListView {
                            id: friendRequestList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip:true

                            model: ListModel {
                                id: friendRequestModel
                            }

                            delegate: Rectangle {
                                width: friendRequestList.width
                                height: Theme.listHeight * 1.5
                                color: "transparent"

                                ColumnLayout {
                                    anchors.fill: parent
                                    anchors.margins: Theme.spacingNormal
                                    spacing: Theme.spacingNormal

                                    RowLayout {
                                        Layout.fillWidth: true

                                        ColumnLayout {
                                            Layout.fillWidth: true
                                            spacing: 2

                                            Label {
                                                text: model.requesterName
                                                font.pixelSize: Theme.fontSizeNormal
                                                font.bold: true
                                                color: Theme.primaryTextColor
                                            }

                                            Label {
                                                text: "ID: " + model.requesterId
                                                font.pixelSize: Theme.fontSizeSmall
                                                color: Theme.secondaryTextColor
                                            }
                                        }
                                    }

                                    Label {
                                        text: model.requestMessage
                                        font.pixelSize: Theme.fontSizeNormal
                                        color: Theme.primaryTextColor
                                        wrapMode: Text.Wrap
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true

                                        Label {
                                            text: model.requestTime
                                            font.pixelSize: Theme.fontSizeSmall
                                            color: Theme.secondaryTextColor
                                        }

                                        Item {
                                            Layout.fillWidth: true
                                        }

                                        Button {
                                            text: "拒绝"
                                            background: Rectangle {
                                                color: "#FF6B6B"
                                                radius: Theme.cornerRadius
                                            }
                                            contentItem: Text {
                                                text: "拒绝"
                                                color: "white"
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: {
                                                // 模拟拒绝好友申请
                                                console.log("已拒绝好友申请: " + model.requesterName)
                                                friendRequestModel.remove(index)
                                                infoText.text = "已拒绝 " + model.requesterName + " 的好友申请"
                                                infoText.color = "red"
                                                infoTimer.start()
                                            }
                                        }

                                        Button {
                                            text: "同意"
                                            background: Rectangle {
                                                color: Theme.primaryColor
                                                radius: Theme.cornerRadius
                                            }
                                            contentItem: Text {
                                                text: "同意"
                                                color: "white"
                                                horizontalAlignment: Text.AlignHCenter
                                                verticalAlignment: Text.AlignVCenter
                                            }
                                            onClicked: {
                                                // 模拟同意好友申请
                                                console.log("已同意好友申请: " + model.requesterName)
                                                friendRequestModel.remove(index)
                                                infoText.text = "已同意 " + model.requesterName + " 的好友申请"
                                                infoText.color = "green"
                                                infoTimer.start()
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // 信息提示文本
            Text {
                id: infoText
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                color: "green"
                opacity: 0
            }

            // 信息提示计时器
            Timer {
                id: infoTimer
                interval: 3000
                onTriggered: {
                    infoText.opacity = 0
                }
            }
        }
    }

    // 当显示信息提示时的动画效果
    NumberAnimation {
        target: infoText
        property: "opacity"
        to: 1
        duration: 300
        onFinished: {
            if (infoText.opacity === 1) {
                infoTimer.start()
            }
        }
    }
}
