import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"
import ".."

Item {
    // 使用RowLayout替代SplitView来实现固定侧边栏
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧侧边栏 (固定宽度，不在SplitView中)
        Item {
            id: leftSidebar
            Layout.preferredWidth: 60
            Layout.minimumWidth: 60
            Layout.maximumWidth: 60
            Layout.fillHeight: true
            clip: true

            // 添加背景色
            Rectangle {
                height: parent.height
                radius: Theme.cornerRadius
                //让矩形比父容器宽，从而将右侧圆角“推”出去
                width: parent.width + radius
                color: Theme.sidebarBackgroundColor
                z: -1 // 确保背景在按钮后面
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.topMargin: 20
                spacing: Theme.spacingLarge // 使用主题间距

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    checkable: true
                    onClicked: {
                        middlePaneStack.currentIndex = 1
                    }

                    contentItem: Image {
                        width: 24
                        height: 24
                        anchors.centerIn: parent

                        // 直接加载 SVG 文件，显示原始颜色
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
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48
                    onClicked: middlePaneStack.currentIndex = 1

                    checkable: true

                    contentItem: Image {
                        width: 24
                        height: 24
                        anchors.centerIn: parent  // 添加居中对齐

                        // 直接加载 SVG 文件，显示原始颜色
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

                // 其他功能按钮可以按同样方式添加
                Item { Layout.fillHeight: true } // 占位符，将退出按钮推到底部

                Button {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 48
                    Layout.preferredHeight: 48

                    onClicked:{
                        Signals.backToLoginPage()// 返回登录页
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

            // 中侧列表面板
            Item {
                id: middlePane
                SplitView.preferredWidth: 280
                SplitView.minimumWidth: 220

                // 添加背景色
                Rectangle {
                    anchors.fill: parent
                    color: Theme.chatBackgroundColor
                }

                StackLayout {
                    id: middlePaneStack
                    anchors.fill: parent
                    currentIndex: 0 // 默认显示会话列表

                    // 会话列表视图 (稍后实现)
                    //ConversationListView { }

                    // 联系人列表视图 (稍后实现)
                    //ContactListView { }
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
}
