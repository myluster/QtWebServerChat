import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../theme"

ListView {
    id: contactView
    clip: true
    spacing: 0

    // 存储当前选中的联系人 ID
    property string currentContactId: ""

    // 使用临时的 ListModel 作为数据源
    model: ListModel {
        ListElement { contactId: "friend_001"; name: "张三"; status: "在线" }
        ListElement { contactId: "friend_002"; name: "李四"; status: "离线" }
        ListElement { contactId: "friend_003"; name: "王五"; status: "在线" }
        ListElement { contactId: "friend_004"; name: "赵六"; status: "离线" }
    }

    // 定义每个列表项的外观和行为
    delegate: Rectangle {
        width: contactView.width
        height: Theme.listHeight
        // 根据当前项是否被选中来设置背景颜色
        color: model.contactId === contactView.currentContactId ? Theme.choiceListColor : "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.margins: Theme.spacingNormal
            spacing: Theme.spacingNormal

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: model.name
                    font.pixelSize: Theme.fontSizeNormal
                    color: Theme.primaryTextColor
                    elide: Text.ElideRight
                }

                Label {
                    text: "ID: " + model.contactId
                    font.pixelSize: Theme.fontSizeSmall
                    color: Theme.secondaryTextColor
                    elide: Text.ElideRight
                }
            }

            // 状态显示（只显示在线/离线）
            Rectangle {
                Layout.preferredWidth: 12
                Layout.preferredHeight: 12
                radius: 6
                color: model.status === "在线" ? Theme.onlineStatusColor : Theme.offlineStatusColor
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                // 如果点击的是已选中的项，则取消选中
                if (contactView.currentContactId === model.contactId) {
                    contactView.currentContactId = ""
                } else {
                    contactView.currentContactId = model.contactId
                }
            }
        }
    }
}
