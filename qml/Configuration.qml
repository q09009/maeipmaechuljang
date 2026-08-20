import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import com.maeip 1.0

Window {
    id: configWindow
    width: 650
    height: 450
    title: "설정"

    signal databaseChanged()

    onClosing: (close) => {
        if (sqlData.transferInProgress)
            close.accepted = false
    }

    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
         | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    Component.onCompleted: {
        var cfg = sqlData.getDbConfig()
        baseUrlField.text = cfg.baseUrl || "http://127.0.0.1:8000"
        apiKeyField.text = cfg.apiKey || ""
        if (cfg.mode === SqlHandler.Api) {
            externalModeRadio.checked = true
        } else {
            localModeRadio.checked = true
        }
    }

    Connections {
        target: sqlData

        function onDataTransferFinished(result) {
            if (result.success) {
                transferStatusText.text = "✅ " + result.message
                        + "\n거래처 " + result.customers + "건 / 품목 " + result.items
                        + "건 / 전표 " + result.records + "건"
                        + "\n원본 백업: " + result.sourceBackup
                        + "\n대상 백업: " + result.targetBackup
                configWindow.databaseChanged()
            } else {
                transferStatusText.text = "❌ " + result.message
                if (result.sourceBackup)
                    transferStatusText.text += "\n원본 백업: " + result.sourceBackup
                if (result.targetBackup)
                    transferStatusText.text += "\n대상 백업: " + result.targetBackup
            }
        }
    }

    Dialog {
        id: transferConfirmDialog
        property int direction: SqlHandler.SqliteToApi
        title: "데이터 전체 이전 확인"
        modal: true
        width: 420
        x: (configWindow.width - width) / 2
        y: (configWindow.height - height) / 2
        standardButtons: Dialog.Ok | Dialog.Cancel

        contentItem: Label {
            text: transferConfirmDialog.direction === SqlHandler.SqliteToApi
                  ? "PostgreSQL의 현재 데이터를 백업한 뒤 SQLite 데이터로 완전히 교체합니다.\n계속하시겠습니까?"
                  : "SQLite의 현재 data.db를 백업한 뒤 PostgreSQL 데이터로 완전히 교체합니다.\n계속하시겠습니까?"
            wrapMode: Text.WordWrap
        }

        onAccepted: {
            transferStatusText.text = "데이터 백업 및 이전을 진행하고 있습니다..."
            sqlData.startDataTransfer(transferConfirmDialog.direction)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 왼쪽 사이드바
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

                Button {
                    text: "닫기"
                    enabled: !sqlData.transferInProgress
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

        // 오른쪽 메인 화면
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
                        text: "서버 모드 (FastAPI)"
                    }

                    // FastAPI 서버 주소
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        visible: externalModeRadio.checked
                        Layout.topMargin: 5

                        Text { text: "API 서버 주소"; font.pixelSize: 12; font.bold: true }
                        TextField {
                            id: baseUrlField
                            placeholderText: "http://127.0.0.1:8000"
                            Layout.fillWidth: true
                            selectByMouse: true
                        }

                        Text { text: "API 키"; font.pixelSize: 12; font.bold: true }
                        TextField {
                            id: apiKeyField
                            placeholderText: "서버의 MAEIP_API_KEY"
                            echoMode: TextInput.Password
                            Layout.fillWidth: true
                            selectByMouse: true
                        }
                    }

                    // 상태 메시지
                    Text {
                        id: statusText
                        text: ""
                        font.pixelSize: 12
                        color: statusText.text.startsWith("✅") ? "#16a34a" : "#dc2626"
                        visible: text !== ""
                    }

                    // 적용 버튼
                    Button {
                        text: "적용"
                        Layout.alignment: Qt.AlignRight
                        onClicked: {
                            if (localModeRadio.checked) {
                                sqlData.setDbMode(SqlHandler.Sqlite)
                                var ok = sqlData.initDB()
                                if (ok) {
                                    statusText.text = "✅ 로컬 SQLite 모드로 전환됐어요."
                                    configWindow.databaseChanged()
                                } else {
                                    statusText.text = "❌ SQLite 초기화 실패: " + sqlData.lastError()
                                }
                            } else {
                                if (baseUrlField.text.trim() === "") {
                                    statusText.text = "❌ API 서버 주소를 입력해주세요."
                                    return
                                }
                                if (apiKeyField.text === "") {
                                    statusText.text = "❌ API 키를 입력해주세요."
                                    return
                                }
                                sqlData.setDbMode(SqlHandler.Api,
                                                  baseUrlField.text.trim(),
                                                  apiKeyField.text)
                                var ok = sqlData.initDB()
                                if (ok) {
                                    statusText.text = "✅ FastAPI 서버에 연결됐어요."
                                    configWindow.databaseChanged()
                                } else {
                                    statusText.text = "❌ 연결 실패: " + sqlData.lastError()
                                }
                            }
                        }
                    }

                    Rectangle { Layout.fillWidth: true; height: 1; color: "#e2e8f0" }

                    Text {
                        text: "SQLite ↔ PostgreSQL 전체 데이터 이전"
                        font.pixelSize: 15
                        font.bold: true
                        color: "#0f172a"
                    }
                    Text {
                        Layout.fillWidth: true
                        text: "대상 데이터베이스의 거래처·품목·전표를 모두 교체합니다. 이전 전에 양쪽 백업을 만들고, 이전 후 건수와 전체 데이터 해시가 일치해야 성공 처리됩니다."
                        wrapMode: Text.WordWrap
                        font.pixelSize: 11
                        color: "#b45309"
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        Button {
                            text: "SQLite → PostgreSQL"
                            enabled: !sqlData.transferInProgress
                            onClicked: {
                                transferConfirmDialog.direction = SqlHandler.SqliteToApi
                                transferConfirmDialog.open()
                            }
                        }
                        Button {
                            text: "PostgreSQL → SQLite"
                            enabled: !sqlData.transferInProgress
                            onClicked: {
                                transferConfirmDialog.direction = SqlHandler.ApiToSqlite
                                transferConfirmDialog.open()
                            }
                        }
                        BusyIndicator {
                            running: sqlData.transferInProgress
                            visible: running
                            Layout.preferredWidth: 28
                            Layout.preferredHeight: 28
                        }
                    }

                    Text {
                        id: transferStatusText
                        Layout.fillWidth: true
                        text: ""
                        visible: text !== ""
                        wrapMode: Text.WrapAnywhere
                        font.pixelSize: 11
                        color: text.startsWith("✅") ? "#16a34a"
                              : text.startsWith("❌") ? "#dc2626" : "#475569"
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
