import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: configWindow
    width: 650
    height: 450
    title: "설정"

    // 👑 핵심: 메인 창 위에 항상 떠 있게 만들고, 부모 창과 모달(Modal)로 묶을지 선택 가능
    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
         | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint
    //modality: Qt.WindowModal // 이 창이 켜져 있는 동안은 메인 창 클릭 안 되게 방어 (원치 않으면 삭제 가능)

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ==========================================
        // 1. 왼쪽 사이드바: 설정 범주 (Category List)
        // ==========================================
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 160
            color: "#f1f5f9"

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                ListView {
                    id: categoryList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    model: ["데이터베이스", "일반 설정", "업데이트"]
                    currentIndex: 0
                    spacing: 4

                    delegate: Button {
                        width: categoryList.width
                        height: 38
                        flat: true

                        background: Rectangle {
                            color: categoryList.currentIndex === index ? "#2563eb" : "transparent"
                            radius: 6
                        }

                        contentItem: Text {
                            text: modelData
                            color: categoryList.currentIndex === index ? "white" : "#334155"
                            font.bold: categoryList.currentIndex === index
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                            leftPadding: 10
                        }

                        onClicked: {
                            categoryList.currentIndex = index
                            contentStack.currentIndex = index
                        }
                    }
                }

                // 닫기 버튼 누르면 윈도우 종료
                Button {
                    text: "닫기"
                    Layout.fillWidth: true
                    height: 35
                    onClicked: configWindow.close()
                }
            }

            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: "#e2e8f0"
            }
        }

        // ==========================================
        // 2. 오른쪽 메인 화면: 설정 상세 내용
        // ==========================================
        StackLayout {
            id: contentStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: categoryList.currentIndex

            // [인덱스 0] 데이터베이스 설정
            ScrollView {
                padding: 25
                ColumnLayout {
                    width: parent.width - 50
                    spacing: 15

                    Text { text: "데이터베이스 연결 설정"; font.pixelSize: 18; font.bold: true; color: "#0f172a" }
                    Text { text: "장부 데이터를 저장할 위치를 선택하세요."; font.pixelSize: 12; color: "#64748b" }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#e2e8f0" }

                    RadioButton {
                        id: localModeRadio
                        text: "로컬 모드 (기본 SQLite)"
                        checked: true
                    }

                    RadioButton {
                        id: externalModeRadio
                        text: "원격 서버 모드 (외부 네트워크 DB)"
                    }

                    // 외부 DB 입력창 (라디오 체크 시 노출)
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: externalModeRadio.checked
                        Layout.topMargin: 5

                        Text { text: "서버 IP / 주소"; font.pixelSize: 12; font.bold: true }
                        TextField { placeholderText: "127.0.0.1"; Layout.fillWidth: true; selectByMouse: true }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 10
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: "사용자 ID"; font.pixelSize: 12; font.bold: true }
                                TextField { Layout.fillWidth: true; selectByMouse: true }
                            }
                            ColumnLayout {
                                Layout.fillWidth: true
                                Text { text: "비밀번호"; font.pixelSize: 12; font.bold: true }
                                TextField { echoMode: TextInput.Password; Layout.fillWidth: true; selectByMouse: true }
                            }
                        }
                    }
                }
            }

            // [인덱스 1] 일반 설정
            Item {
                Text { anchors.centerIn: parent; text: "일반 설정 준비 중"; font.pixelSize: 14; color: "#64748b" }
            }

            // [인덱스 2] 업데이트
            Item {
                Text {
                    anchors.centerIn: parent
                    text: "현재 버전: " + CURRENT_VERSION + "\n최신 버전: " + latestVersionStr
                    font.pixelSize: 14
                    color: "#0f172a"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }
}
