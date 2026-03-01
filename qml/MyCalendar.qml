import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    width: 270
    height: 300
    color: "#ffffff"
    radius: 8
    border.color: "#e2e6ea"
    border.width: 1

    readonly property color calPrimary: "#2563eb"
    readonly property color calMuted: "#64748b"
    readonly property color calBorder: "#e2e6ea"
    readonly property color calHover: "#f1f5f9"

    property date selectedDate: new Date()
    property int year: selectedDate.getFullYear()
    property int month: selectedDate.getMonth() + 1
    //property int calendarParent

    signal dateSelected(date date)
    signal closeRequested()

    function changeMonth(step) {
        var newDate = new Date(year, month - 1 + step, 1)
        year = newDate.getFullYear()
        month = newDate.getMonth() + 1
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // 헤더 (연도 + 월 + 닫기)
        RowLayout {
            Layout.fillWidth: true
            spacing: 4

            Button {
                text: "◀"
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                flat: true
                onClicked: root.year--
                contentItem: Text { text: parent.text; font.pixelSize: 14; color: root.calMuted; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.hovered ? root.calHover : "transparent"; radius: 6 }
            }
            Text {
                text: qsTr("%1년").arg(root.year)
                font.bold: true
                font.pixelSize: 14
                color: root.calMuted
                Layout.preferredWidth: 44
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                text: "▶"
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                flat: true
                onClicked: root.year++
                contentItem: Text { text: parent.text; font.pixelSize: 14; color: root.calMuted; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.hovered ? root.calHover : "transparent"; radius: 6 }
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "‹"
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                flat: true
                onClicked: changeMonth(-1)
                contentItem: Text { text: parent.text; font.pixelSize: 18; color: root.calPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.hovered ? root.calHover : "transparent"; radius: 6 }
            }
            Text {
                text: qsTr("%1월").arg(root.month)
                font.bold: true
                font.pixelSize: 14
                color: root.calPrimary
                Layout.preferredWidth: 32
                horizontalAlignment: Text.AlignHCenter
            }
            Button {
                text: "›"
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                flat: true
                onClicked: changeMonth(1)
                contentItem: Text { text: parent.text; font.pixelSize: 18; color: root.calPrimary; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                background: Rectangle { color: parent.hovered ? root.calHover : "transparent"; radius: 6 }
            }

            Item { Layout.preferredWidth: 4 }
            Button {
                text: "✕"
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                flat: true
                onClicked: {
                    // if(calendarParent === 0) calendarPopup.close()
                    // else if(calendarParent === 1) scalendarPopup1.close()
                    // else if(calendarParent === 2) scalendarPopup2.close()
                    root.closeRequested() // "나 닫고 싶어!"라고 신호만 보냄
                }
                contentItem: Text {
                    text: parent.text
                    color: root.calMuted
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle { color: parent.hovered ? "#fef2f2" : "transparent"; radius: 6 }
            }
        }

        Rectangle { Layout.fillWidth: true; height: 1; color: root.calBorder }

        // 요일 헤더
        DayOfWeekRow {
            Layout.fillWidth: true
            locale: Qt.locale("ko_KR")
            delegate: Text {
                text: model.shortName
                font.bold: true
                font.pixelSize: 12
                color: model.index === 0 ? "#dc2626" : (model.index === 6 ? root.calPrimary : root.calMuted)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
        }

        // 날짜 그리드
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

                opacity: isCurrentMonth ? 1.0 : 0.35
                color: isSelected ? root.calPrimary : "transparent"
                radius: width / 2
                border.color: !isSelected && isToday ? root.calPrimary : "transparent"
                border.width: 1.5

                Text {
                    anchors.centerIn: parent
                    text: model.date.getDate()
                    color: isSelected ? "white" : "#334155"
                    font.pixelSize: 13
                    font.bold: isSelected || isToday
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        root.selectedDate = model.date
                        // if(calendarParent === 0) calendarButton.currentDate = model.date
                        // else if(calendarParent === 1) searchCalendarFirst.currentDate = model.date
                        // else if(calendarParent === 2) searchCalendarSecond.currentDate = model.date
                        root.dateSelected(model.date)
                    }
                }
            }
        }
    }
}
