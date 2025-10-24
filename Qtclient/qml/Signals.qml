import QtQuick

pragma Singleton

QtObject {
    // 进入主页面信号
    signal goToMainPage()

    // 返回登录页面信号
    signal backToLoginPage()

    // 全屏状态改变信号
    signal fullscreenChanged(bool isFullscreen)

    // 打开添加好友窗口信号
    signal openAddFriendWindow()
}
