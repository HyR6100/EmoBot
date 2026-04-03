import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml 2.15

ApplicationWindow {
    id: root
    width: 1100
    height: 680
    visible: true
    title: "EmoBot 梦幻陪伴系统"

    property bool busy: false

    ListModel { id: msgModel }

    // A soft pink dreamy background (simple gradients for MVP).
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: "#110018" }
            GradientStop { position: 0.45; color: "#2a0038" }
            GradientStop { position: 1.0; color: "#16001f" }
        }
    }

    Rectangle {
        id: topBar
        height: 72
        anchors.top: parent.top
        width: parent.width
        color: "#220020"
        opacity: 0.92

        RowLayout {
            anchors.fill: parent
            anchors.margins: 16

            Label {
                text: "EmoBot Companion"
                color: "#ffd1ff"
                font.pixelSize: 20
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: "新对话"
                onClicked: {
                    msgModel.clear()
                    backend.resetConversation()
                    busy = false
                    statusLabel.text = "已重置对话"
                    lastMetaLabel.text = ""
                }
            }
        }
    }

    ColumnLayout {
        anchors.top: topBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        spacing: 0

        Rectangle {
            id: statusBar
            Layout.fillWidth: true
            height: 44
            color: "#2a0038"
            opacity: 0.88

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10

                Label {
                    id: statusLabel
                    text: "就绪"
                    color: "#ffd1ff"
                    Layout.fillWidth: true
                }

                Label {
                    id: lastMetaLabel
                    text: ""
                    color: "#ffb3ff"
                    font.pixelSize: 12
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 12
            anchors.margins: 14

            // Chat area
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#110018"
                opacity: 0.75
                radius: 18
                border.color: "#ff66ff"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 14
                    spacing: 10

                    ListView {
                        id: listView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: msgModel
                        clip: true
                        spacing: 10

                        delegate: Item {
                            width: listView.width
                            height: contentText.paintedHeight + 18

                            Rectangle {
                                id: bubble
                                width: listView.width * 0.78
                                radius: 16
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.left: role === "user" ? undefined : parent.left
                                anchors.right: role === "user" ? parent.right : undefined
                                anchors.leftMargin: role === "user" ? 0 : 0
                                anchors.rightMargin: role === "user" ? 0 : 0

                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: role === "user" ? "#ff6fd6" : "#5b2cff"
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: role === "user" ? "#ffb7f5" : "#b29bff"
                                    }
                                }
                                opacity: 0.95

                                border.color: "#ffffff"
                                border.width: 1

                                Text {
                                    id: contentText
                                    anchors.fill: parent
                                    anchors.margins: 10
                                    wrapMode: Text.Wrap
                                    color: "white"
                                    font.pixelSize: 15
                                    text: modelText
                                }
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        TextArea {
                            id: input
                            Layout.fillWidth: true
                            height: 90
                            wrapMode: Text.Wrap
                            placeholderText: "输入你的话..."
                            enabled: !busy
                        }

                        Button {
                            id: sendBtn
                            width: 110
                            text: busy ? "中..." : "发送"
                            enabled: !busy
                            onClicked: {
                                const t = input.text.trim()
                                if (t.length === 0) return

                                msgModel.append({ role: "user", modelText: t })
                                input.text = ""
                                busy = true
                                statusLabel.text = "思考中..."
                                backend.sendMessage(t, personaArea.text)
                            }
                        }
                    }
                }
            }

            // Persona editor area
            Rectangle {
                width: 360
                Layout.fillHeight: true
                color: "#100016"
                opacity: 0.75
                radius: 18
                border.color: "#ff66ff"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 8

                    Label {
                        text: "人物性格（自定义）"
                        color: "#ffd1ff"
                        font.pixelSize: 14
                        font.bold: true
                    }

                    TextArea {
                        id: personaArea
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        wrapMode: Text.Wrap
                        text: backend.defaultPersonaText()
                    }

                    Label {
                        text: "提示：你只需要改“人物设定/说话风格”，输出格式会自动约束为 JSON。"
                        color: "#ffb3ff"
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }
                }
            }
        }
    }

    Connections {
        target: backend
        onAssistantReplyReady: {
            msgModel.append({ role: "assistant", modelText: reply })
            lastMetaLabel.text = "emotion=" + emotion + " gesture=" + gesture
            statusLabel.text = "已回复"
            busy = false
        }
        onStatusChanged: {
            // Only show when not busy; keeps UX stable.
            statusLabel.text = status
            if (status.indexOf("思考中") !== -1)
                busy = true
        }
        onErrorOccurred: {
            statusLabel.text = message
            busy = false
        }
    }
}

