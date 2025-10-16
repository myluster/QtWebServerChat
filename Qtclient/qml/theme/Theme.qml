import QtQuick

// 将此文件注册为一个全局单例对象
pragma Singleton

QtObject {
    // --- 调色板 ---
    readonly property color primaryColor: "#07C160"             // 微信主绿色 (用于按钮、高亮等)
    readonly property color backgroundColor: "#EDEDED"          // 主背景色
    readonly property color chatBackgroundColor: "#EDEDED"      // 聊天窗口背景色
    readonly property color outgoingBubbleColor: "#95EC69"      // 发送气泡颜色
    readonly property color incomingBubbleColor: "#FFFFFF"      // 接收气泡颜色
    readonly property color primaryTextColor: "#000000"         // 主要文本颜色
    readonly property color secondaryTextColor: "#888888"       // 次要文本颜色 (如时间戳)
    readonly property color sidebarBackgroundColor:"#EDEDED"    //侧边栏背景色
    readonly property color dividerColor: "#D5D5D5"             // 分割线颜色
    readonly property color buttonHoverColor: "#E1E1E1"         //按钮悬停颜色

    // --- 字体大小 ---
    readonly property int fontSizeSmall: 12
    readonly property int fontSizeNormal: 14
    readonly property int fontSizeLarge: 16
    readonly property int fontSizeTitle: 18

    // --- 间距与边距 ---
    readonly property int spacingSmall: 4
    readonly property int spacingNormal: 8
    readonly property int spacingLarge: 16

    // --- 圆角半径 ---
    readonly property int cornerRadius: 8
}
