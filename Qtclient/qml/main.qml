import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import "."

ApplicationWindow {
    id: rootWindow

    width: 400
    height: 600
    minimumWidth: 400
    minimumHeight: 600
    visible: true
    title: "QtClient"

    // 保存窗口状态的属性
    property int normalWidth: 400
    property int normalHeight: 600
    property int normalX: 0
    property int normalY: 0

    // 移除原生边框，并使窗口背景完全透明
    flags: Qt.Window | Qt.FramelessWindowHint
    color: "transparent"

    // 背景和阴影层
    // 这个矩形只负责视觉表现：背景色、边框和阴影。
    Rectangle {
        id: backgroundAndShadow
        anchors.fill: parent
        anchors.margins: rootWindow.visibility === ApplicationWindow.Maximized ? 0 : 10 // 为阴影留出空间
        color: "white"
        radius: rootWindow.visibility === ApplicationWindow.Maximized ? 0 : Theme.cornerRadius
        border.color: "#E0E0E0"
        border.width: 1

        // 阴影效果
        layer.enabled: rootWindow.visibility !== ApplicationWindow.Maximized
        layer.smooth: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 0
            radius: Theme.cornerRadius
            samples: 2*Theme.cornerRadius+1
            color: "#80000000"
        }
    }

    // 内容裁剪层 (透明，但负责裁剪)
    // 这个矩形的大小和位置与背景层完全相同，它的唯一作用就是裁剪内部的 StackView。
    Rectangle {
        id: contentClipper
        anchors.fill: parent
        anchors.margins: rootWindow.visibility === ApplicationWindow.Maximized ? 0 : 10
        radius: rootWindow.visibility === ApplicationWindow.Maximized ? 0 : Theme.cornerRadius
        color: "transparent" // 本身是看不见的
        clip: true           // 在这里进行内容裁剪

        // 所有UI内容都应放在这个裁剪矩形内部
        StackView {
            id: rootStack
            anchors.fill: parent
            initialItem: "qrc:/qml/views/LoginPage.qml"

            // 推入(push)页面的过渡效果
            pushEnter: Transition {
                PropertyAnimation {
                    property: "x"
                    from: rootStack.width
                    to: 0
                    duration: 500
                    easing.type: Easing.InOutQuad
                }
            }

            pushExit: Transition {
                PropertyAnimation {
                    property: "x"
                    from: 0
                    to: -rootStack.width
                    duration: 500
                    easing.type: Easing.InOutQuad
                }
            }

            // 弹出(pop)页面的过渡效果
            popEnter: Transition {
                PropertyAnimation {
                    property: "x"
                    from: -rootStack.width
                    to: 0
                    duration: 450
                    easing.type: Easing.InOutQuad
                }
            }

            popExit: Transition {
                PropertyAnimation {
                    property: "x"
                    from: 0
                    to: rootStack.width
                    duration: 450
                    easing.type: Easing.InOutQuad
                }
            }

            // 替换(replace)页面的过渡效果 (原样保留)
            replaceEnter: Transition {
                PropertyAnimation {
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 0
                }
            }

            replaceExit: Transition {
                PropertyAnimation {
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 0
                }
            }
        }
        // 窗口控制按钮容器 - 放置在右上角
        Row {
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            spacing: 5
            z: 1000 // 确保按钮在最上层

            // 最小化按钮
            Button {
                width: 30
                height: 30

                contentItem: Image {
                    source: "qrc:/qml/assets/subtract-line.svg"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                }

                background: Rectangle {
                    color: parent.hovered ? "#D5D5D5" : "transparent"
                    radius: Theme.cornerRadius
                }

                onClicked: {
                    rootWindow.showMinimized()
                }
            }

            // 全屏/还原按钮
            Button {
                width: 30
                height: 30

                contentItem: Image {
                    source: rootWindow.visibility === ApplicationWindow.Maximized ?
                                "qrc:/qml/assets/fullscreen-exit-line.svg" :
                                "qrc:/qml/assets/fullscreen-line.svg"
                    fillMode: Image.PreserveAspectFit
                    sourceSize.width: 16
                }

                background: Rectangle {
                    color: parent.hovered ? "#D5D5D5" : "transparent"
                    radius: Theme.cornerRadius
                }

                onClicked: {
                    toggleFullscreen()//进入退出全屏都可处理
                }
            }

            // 关闭按钮
            Button {
                width: 30
                height: 30

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
                    rootWindow.close()
                }
            }
        }

        // 窗口拖拽区域 - 标题栏区域
        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 40
            z: 998   // 在内容之上但在控制按钮之下

            onPressed: {
                rootWindow.startSystemMove()
            }
        }

        // 窗口边缘调整大小区域
        // 左边缘
        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 10
            cursorShape: Qt.SizeHorCursor
            z: 999//在拖拽区域之上

            onPressed: {
                rootWindow.startSystemResize(Qt.LeftEdge)
            }
        }

        // 右边缘
        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 10
            cursorShape: Qt.SizeHorCursor
            z: 999

            onPressed: {
                rootWindow.startSystemResize(Qt.RightEdge)
            }
        }

        // 上边缘
        MouseArea {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 10
            cursorShape: Qt.SizeVerCursor
            z: 999

            onPressed: {
                rootWindow.startSystemResize(Qt.TopEdge)
            }
        }

        // 下边缘
        MouseArea {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 10
            cursorShape: Qt.SizeVerCursor
            z: 999

            onPressed: {
                rootWindow.startSystemResize(Qt.BottomEdge)
            }
        }

        // 左上角
        MouseArea {
            anchors.left: parent.left
            anchors.top: parent.top
            width: 10
            height: 10
            cursorShape: Qt.SizeFDiagCursor
            z: 999

            onPressed: {
                rootWindow.startSystemResize(Qt.TopLeftCorner)
            }
        }

        // 右上角
        MouseArea {
            anchors.right: parent.right
            anchors.top: parent.top
            width: 10
            height: 10
            cursorShape: Qt.SizeBDiagCursor
            z: 999

            onPressed: {
                rootWindow.startSystemResize(Qt.TopRightCorner)
            }
        }

        // 左下角
        MouseArea {
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            width: 10
            height: 10
            cursorShape: Qt.SizeBDiagCursor
            z: 999

            onPressed: {
                rootWindow.startSystemResize(Qt.BottomLeftCorner)
            }
        }

        // 右下角
        MouseArea {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 10
            height: 10
            cursorShape: Qt.SizeFDiagCursor
            z: 999

            onPressed: {
                rootWindow.startSystemResize(Qt.BottomRightCorner)
            }
        }
    }

    onWidthChanged: {
        if (rootWindow.visibility !== ApplicationWindow.Maximized) {
            normalWidth = rootWindow.width
        }
    }

    onHeightChanged: {
        if (rootWindow.visibility !== ApplicationWindow.Maximized) {
            normalHeight = rootWindow.height
        }
    }

    onXChanged: {
        if (rootWindow.visibility !== ApplicationWindow.Maximized) {
            normalX = rootWindow.x
        }
    }

    onYChanged: {
        if (rootWindow.visibility !== ApplicationWindow.Maximized) {
            normalY = rootWindow.y
        }
    }

    // 切换全屏状态的函数
    function toggleFullscreen() {
        if (rootWindow.visibility == ApplicationWindow.Maximized) {
            // 退出全屏，恢复到之前的窗口状态
            rootWindow.visibility = ApplicationWindow.Windowed
            rootWindow.x = normalX
            rootWindow.y = normalY
            rootWindow.width = normalWidth
            rootWindow.height = normalHeight
        } else {
            // 进入全屏前保存当前窗口状态
            normalX = rootWindow.x
            normalY = rootWindow.y
            normalWidth = rootWindow.width
            normalHeight = rootWindow.height

            // 进入全屏
            rootWindow.visibility = ApplicationWindow.Maximized
        }
        // 发出全屏状态改变信号
        Signals.fullscreenChanged(rootWindow.visibility === ApplicationWindow.Maximized)
    }

    Connections{
        target:Signals

        // 进入主页面
        function onGoToMainPage()
        {
            // 登录成功后调整窗口大小
            normalWidth = 1024
            normalHeight = 768
            // 非全屏大小下才正式改变可视窗口大小
            if(rootWindow.visibility != ApplicationWindow.Maximized){
                rootWindow.width = 1024
                rootWindow.height = 768
            }
            rootStack.replace("qrc:/qml/views/MainPage.qml")
        }

        // 处理返回登录页面请求
        function onBackToLoginPage() {
            // 如果当前退出全屏的窗口大小不是登录页面的默认大小，则重置为默认大小 {
            normalWidth = 400
            normalHeight = 600
            // 非全屏大小下才正式改变可视窗口大小
            if(rootWindow.visibility != ApplicationWindow.Maximized){
                rootWindow.width = 400
                rootWindow.height = 600
            }
            rootStack.replace("qrc:/qml/views/LoginPage.qml")
        }

        // 处理打开添加好友窗口请求
        function onOpenAddFriendWindow() {
            // 创建添加好友窗口
            var component = Qt.createComponent("qrc:/qml/views/AddFriendWindow.qml")
            if (component.status === Component.Ready) {
                var window = component.createObject(rootWindow, {
                    "width": 400,
                    "height": 300,
                    "title": "添加好友"
                })
                window.show()
            } else {
                console.error("无法加载添加好友窗口:", component.errorString())
            }
        }
    }
}
