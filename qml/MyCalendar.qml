import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 270
    // 헤더가 한 줄로 줄었으므로 높이를 다시 조정합니다.
    height: 300
    color: "white"
    radius: 5
    border.color: "#ddd"

    property date selectedDate: new Date()
    property int year: selectedDate.getFullYear()
    property int month: selectedDate.getMonth() + 1
    property int calendarParent

    // 내부 로직용 함수
    function changeMonth(step) {
        var newDate = new Date(year, month - 1 + step, 1)
        year = newDate.getFullYear()
        month = newDate.getMonth() + 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 5

        // ==================================================================
        // 1. 헤더 (연도 + 월 + 닫기 버튼을 한 줄에 배치)
        // ==================================================================
        RowLayout {
            Layout.fillWidth: true
            spacing: 5

            // --- 연도 섹션 ---
            Button {
                text: "◀"
                Layout.preferredWidth: 25; Layout.preferredHeight: 30
                flat: true
                onClicked: root.year--
                background: Rectangle { color: parent.down ? "#eee" : "transparent"; radius: 15 }
            }
            Text {
                text: qsTr("%1년").arg(root.year)
                font.bold: true; font.pixelSize: 15; color: "#333"
                Layout.preferredWidth: 45 // 너비 고정
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                text: "▶"
                Layout.preferredWidth: 25; Layout.preferredHeight: 30
                flat: true
                onClicked: root.year++
                background: Rectangle { color: parent.down ? "#eee" : "transparent"; radius: 15 }
            }

            // --- 중앙 여백 ---
            Item { Layout.fillWidth: true }

            // --- 월 섹션 ---
            Button {
                text: "‹"
                Layout.preferredWidth: 25; Layout.preferredHeight: 30
                flat: true
                onClicked: changeMonth(-1)
                contentItem: Text { text: parent.text; font.pixelSize: 20; color: "#1976D2"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.down ? "#eee" : "transparent"; radius: 15 }
            }
            Text {
                text: qsTr("%1월").arg(root.month)
                font.bold: true; font.pixelSize: 15; color: "#1976D2"
                Layout.preferredWidth: 35 // 너비 고정
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                text: "›"
                Layout.preferredWidth: 25; Layout.preferredHeight: 30
                flat: true
                onClicked: changeMonth(1)
                contentItem: Text { text: parent.text; font.pixelSize: 20; color: "#1976D2"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.down ? "#eee" : "transparent"; radius: 15 }
            }

            // --- 닫기 버튼 ---
            Item { Layout.preferredWidth: 5 } // 여백
            Button {
                text: "✕"
                Layout.preferredWidth: 25; Layout.preferredHeight: 30
                flat: true
                onClicked: {
                    if(calendarParent === 0) calendarPopup.close()
                    else if(calendarParent === 1) scalendarPopup1.close()
                    else if(calendarParent === 2) scalendarPopup2.close()
                }
                contentItem: Text {
                    text: parent.text; color: "#888"; font.bold: true
                    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: parent.down ? "#ffebee" : "transparent"; radius: 15 }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: "#eee" } // 구분선

        // ==================================================================
        // 2. 요일 헤더
        // ==================================================================
        DayOfWeekRow {
            Layout.fillWidth: true
            locale: Qt.locale("ko_KR")
            delegate: Text {
                text: model.shortName
                font.bold: true; font.pixelSize: 13
                color: model.index === 0 ? "red" : (model.index === 6 ? "blue" : "#666")
                horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter
            }
        }

        // ==================================================================
        // 3. 날짜 그리드
        // ==================================================================
        MonthGrid {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true

            year: root.year
            month: root.month - 1
            locale: Qt.locale("ko_KR")

            delegate: Rectangle {
                property bool isCurrentMonth: model.date.getMonth() === grid.month
                property bool isSelected: model.date.toDateString() === root.selectedDate.toDateString()
                property bool isToday: model.date.toDateString() === new Date().toDateString()

                opacity: isCurrentMonth ? 1.0 : 0.3
                color: isSelected ? "#1976D2" : "transparent"
                radius: width / 2
                border.color: !isSelected && isToday ? "#1976D2" : "transparent"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: model.date.getDate()
                    color: isSelected ? "white" : "#333"
                    font.bold: isSelected || isToday
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.selectedDate = model.date

                        // 데이터 갱신만 하고 팝업은 닫지 않음
                        if(calendarParent === 0) calendarButton.currentDate = model.date
                        else if(calendarParent === 1) searchCalendarFirst.currentDate = model.date
                        else if(calendarParent === 2) searchCalendarSecond.currentDate = model.date
                    }
                }
            }
        }
    }
}
