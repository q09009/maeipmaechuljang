import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Window {
    id: mtWindow
    width: 680 // 표 형태라 너비를 조금 더 확보했습니다
    height: 720
    visible: false
    title: "월별 통계"

    flags: Qt.Window | Qt.WindowTitleHint | Qt.WindowSystemMenuHint
         | Qt.WindowMinMaxButtonsHint | Qt.WindowCloseButtonHint

    // 0값 처리를 위한 포맷 함수 (값이 없거나 0이면 "0" 반환)
    function formatNum(val) {
        return (Number(val) || 0).toLocaleString(Qt.locale(), 'f', 0);
    }

    Rectangle {
        id: mtroot
        anchors.fill: parent
        color: "#f0f2f5" // 전체 배경색 (연한 회색톤으로 깔끔하게)

        property var mtModel: [];
        property var mtBungi: [];
        property var mtBangi: [];

        property bool mtGB

        // 엑셀 셀처럼 보이는 공통 컴포넌트
        component ExcelCell: Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"     // 기본 셀 배경색
            border.color: "#bdc3c7" // 테두리 색상 (연한 회색)
            border.width: 1

            property alias text: label.text
            property alias textColor: label.color
            property alias fontBold: label.font.bold
            property string align: "right" // 기본 정렬 우측

            Text {
                id: label
                anchors.fill: parent
                anchors.rightMargin: 5
                anchors.leftMargin: 5
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: parent.align === "center" ? Text.AlignHCenter : Text.AlignRight
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 15
            spacing: 10

            // =========================================================
            // [검색 조건 영역] - 엑셀 상단 제어바 느낌
            // =========================================================
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 50
                color: "#E3F2FD" // 이미지의 상단 민트/베이지 느낌을 살짝 준 배경 (원하면 흰색으로 변경 가능)
                border.color: "#bdc3c7"
                radius: 5

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 10
                    spacing: 10

                    // 라벨 추가하여 이미지와 비슷하게 구성 (원치 않으면 Text 부분 삭제)
                    Text { text: "연도"; font.bold: true }
                    ComboBox {
                        id: mtYearComboBox
                        Layout.preferredWidth: 80
                        Layout.preferredHeight: 25
                        model: Array.from({length: 41}, (v, i) => (2010 + i).toString())
                        currentIndex: 0
                        // (기존 팝업 로직 유지)
                        popup: Popup {
                            y: mtYearComboBox.height - 1
                            width: mtYearComboBox.width
                            height: Math.min(contentItem.implicitHeight, 200)
                            padding: 1
                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: mtYearComboBox.popup.visible ? mtYearComboBox.delegateModel : null
                                currentIndex: mtYearComboBox.highlightedIndex
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                            }
                        }
                    }

                    Text { text: "구분"; font.bold: true }
                    ComboBox {
                        id: mtGBComboBox
                        Layout.preferredWidth: 70
                        Layout.preferredHeight: 25
                        model: ["매입", "매출"]
                        currentIndex: 0
                        // (기존 팝업 로직 유지)
                        popup: Popup {
                            y: mtGBComboBox.height - 1
                            width: mtGBComboBox.width
                            height: Math.min(contentItem.implicitHeight, 200)
                            padding: 1
                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: mtGBComboBox.popup.visible ? mtGBComboBox.delegateModel : null
                                currentIndex: mtGBComboBox.highlightedIndex
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                            }
                        }
                    }

                    Text { text: "거래처/상품"; font.bold: true }
                    ComboBox {
                        id: mtSupplierComboBox
                        Layout.preferredWidth: 120
                        Layout.preferredHeight: 25
                        model: mainWindow.supplierSearchList
                        currentIndex: 0
                        // (기존 팝업 로직 유지)
                        popup: Popup {
                            y: mtSupplierComboBox.height - 1
                            width: mtSupplierComboBox.width
                            height: Math.min(contentItem.implicitHeight, 300)
                            padding: 1
                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: mtSupplierComboBox.popup.visible ? mtSupplierComboBox.delegateModel : null
                                currentIndex: mtSupplierComboBox.highlightedIndex
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                            }
                        }
                    }

                    ComboBox {
                        id: mtProductComboBox
                        Layout.fillWidth: true // 남은 공간 차지
                        Layout.preferredHeight: 25
                        model: mainWindow.productSearchList
                        currentIndex: 0
                         // (기존 팝업 로직 유지)
                         popup: Popup {
                            y: mtProductComboBox.height - 1
                            width: mtProductComboBox.width
                            height: Math.min(contentItem.implicitHeight, 300)
                            padding: 1
                            contentItem: ListView {
                                clip: true
                                implicitHeight: contentHeight
                                model: mtProductComboBox.popup.visible ? mtProductComboBox.delegateModel : null
                                currentIndex: mtProductComboBox.highlightedIndex
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                            }
                        }
                    }

                    Button {
                        id: mtButton
                        text: qsTr("검색")
                        Layout.preferredHeight: 25
                        // (기존 클릭 로직 그대로 유지)
                        onClicked: {
                            if(mtGBComboBox.currentText === "매입") {
                                mtroot.mtGB = true;
                            }
                            else {
                                mtroot.mtGB = false;
                            }

                            excelData.getMonthTotal(mtYearComboBox.currentText, mtGBComboBox.currentText, mtSupplierComboBox.currentText, mtProductComboBox.currentText);

                            mtroot.mtModel = [];
                            mtroot.mtBungi = [];
                            mtroot.mtBangi = [];

                            var tempModel = [];
                            var tbun = [];
                            var tban = [];
                            var tempa = excelData.getMTAmount();
                            var tempg = excelData.getMTGongga();
                            var tempb = excelData.getMTBuga();
                            var temph = excelData.getMTHapgye();
                            if(mtGBComboBox.currentText === "매입") {
                                var tempm = excelData.getMTMiji();
                            }
                            else {
                                var tempm = excelData.getMTMisu();
                            }
                            //var tempm = excelData.getMTMisu();


                            var tempbungia=0; var tempbungig=0; var tempbungib=0; var tempbungih=0; var tempbungim=0;
                            var tempbangia=0; var tempbangig=0; var tempbangib=0; var tempbangih=0; var tempbangim=0;
                            var bangiasum=0; var bangigsum=0; var bangibsum=0; var bangihsum=0; var bangimsum=0;
                            var tempbangi = ["상반기", "하반기", "총합"];

                            for(let i=0; i<tempa.length; i++) {
                                let a = tempa[i] || 0; let g = tempg[i] || 0; let b = tempb[i] || 0; let h = temph[i] || 0; let m = tempm[i] || 0;
                                tempModel.push({amount: a, gongga: g, buga: b, hapgye: h, misu: m})

                                tempbungia += a; tempbungig += g; tempbungib += b; tempbungih += h; tempbungim += m;
                                tempbangia += a; tempbangig += g; tempbangib += b; tempbangih += h; tempbangim += m;

                                if((i+1) % 3 == 0) {
                                    tbun.push({amount: tempbungia, gongga: tempbungig, buga: tempbungib, hapgye: tempbungih, misu: tempbungim})
                                    tempbungia = 0; tempbungig = 0; tempbungib = 0; tempbungih = 0; tempbungim = 0;
                                }
                                if((i+1) % 6 == 0) {
                                    tban.push({amount: tempbangia, gongga: tempbangig, buga: tempbangib, hapgye: tempbangih, misu: tempbangim, bg: tempbangi[(i+1)/6-1]})
                                    bangiasum += tempbangia; bangigsum += tempbangig; bangibsum += tempbangib; bangihsum += tempbangih; bangimsum += tempbangim;
                                    tempbangia = 0; tempbangig = 0; tempbangib = 0; tempbangih = 0; tempbangim = 0;
                                }
                            }
                            tban.push({amount: bangiasum, gongga: bangigsum, buga: bangibsum, hapgye: bangihsum, misu: bangimsum, bg: tempbangi[2]})

                            mtroot.mtModel = tempModel;
                            mtroot.mtBungi = tbun;
                            mtroot.mtBangi = tban;
                        }
                    }
                }
            }

            // =========================================================
            // [데이터 테이블 영역]
            // =========================================================

            // 헤더 (컬럼명)
            RowLayout {
                spacing: -1 // 테두리 겹치게 하여 이중선 방지
                Layout.fillWidth: true
                Layout.preferredHeight: 30

                // 헤더용 스타일 (배경색 베이지/살구색)
                component HeaderCell: ExcelCell {
                    color: "#E8F5E9" // 이미지의 베이지색 헤더 느낌
                    align: "center"
                    fontBold: true
                }

                HeaderCell { id: mtgb; text: "구분"; Layout.preferredWidth: 80 }
                HeaderCell { id: mtam; text: "수량"; Layout.preferredWidth: 80 }
                HeaderCell { id: mtgg; text: "공급가액"; Layout.preferredWidth: 130 }
                HeaderCell { id: mtbg; text: "부가세액"; Layout.preferredWidth: 100 }
                HeaderCell { id: mthg; text: "합계금액"; Layout.preferredWidth: 130 }
                HeaderCell { id: mtms; text: `미${mtroot.mtGB ? "지급" : "수금"}액`; Layout.preferredWidth: 130 }
            }

            // 1. 월별 리스트
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 12 * 25 // 12개월 * 높이
                clip: true
                interactive: false // 스크롤 막고 전체 다 보여주기 (엑셀 표처럼)
                model: mtroot.mtModel
                spacing: -1 // 테두리 겹침

                delegate: RowLayout {
                    width: parent.width
                    height: 25
                    spacing: -1

                    ExcelCell { text: (index + 1) + "월"; Layout.preferredWidth: 80; align: "center"; color: "#f9fbe7" } // 월 컬럼 약간 다른 색
                    ExcelCell { text: formatNum(modelData.amount); Layout.preferredWidth: 80 }
                    ExcelCell { text: formatNum(modelData.gongga); Layout.preferredWidth: 130 }
                    ExcelCell { text: formatNum(modelData.buga); Layout.preferredWidth: 100 }
                    ExcelCell { text: formatNum(modelData.hapgye); Layout.preferredWidth: 130 }
                    ExcelCell { text: formatNum(modelData.misu); Layout.preferredWidth: 130 }
                }
            }

            // 간격
            Item { Layout.preferredHeight: 10 }

            // 2. 분기별 리스트
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 4 * 25
                clip: true
                interactive: false
                model: mtroot.mtBungi
                spacing: -1

                delegate: RowLayout {
                    width: parent.width
                    height: 25
                    spacing: -1

                    ExcelCell { text: (index + 1) + "/4분기"; Layout.preferredWidth: 80; align: "center"; color: "#e0f2f1" } // 분기 컬럼 색상
                    ExcelCell { text: formatNum(modelData.amount); Layout.preferredWidth: 80 }
                    ExcelCell { text: formatNum(modelData.gongga); Layout.preferredWidth: 130 }
                    ExcelCell { text: formatNum(modelData.buga); Layout.preferredWidth: 100 }
                    ExcelCell { text: formatNum(modelData.hapgye); Layout.preferredWidth: 130 }
                    ExcelCell { text: formatNum(modelData.misu); Layout.preferredWidth: 130 }
                }
            }

            // 간격
            Item { Layout.preferredHeight: 10 }

            // 3. 반기/연간 리스트
            ListView {
                Layout.fillWidth: true
                Layout.preferredHeight: 3 * 25
                clip: true
                interactive: false
                model: mtroot.mtBangi
                spacing: -1

                delegate: RowLayout {
                    width: parent.width
                    height: 25
                    spacing: -1

                    ExcelCell {
                        text: modelData.bg;
                        Layout.preferredWidth: 80;
                        align: "center";
                        color: "#e1f5fe"; // 반기 컬럼 색상
                        fontBold: true
                    }
                    ExcelCell { text: formatNum(modelData.amount); Layout.preferredWidth: 80; fontBold: true }
                    ExcelCell { text: formatNum(modelData.gongga); Layout.preferredWidth: 130; fontBold: true }
                    ExcelCell { text: formatNum(modelData.buga); Layout.preferredWidth: 100; fontBold: true }
                    ExcelCell { text: formatNum(modelData.hapgye); Layout.preferredWidth: 130; fontBold: true }
                    ExcelCell { text: formatNum(modelData.misu); Layout.preferredWidth: 130; fontBold: true }
                }
            }

            // 하단 안내 문구
            Text {
                Layout.topMargin: 5
                text: "* 수량, 공급가액, 부가세액, 합계금액, 미지급액/미수금액은 거래일을 기준으로 산정됩니다."
                color: "red"
                font.pixelSize: 11
            }

            // 남은 공간 채우기
            Item { Layout.fillHeight: true }
        }
    }
}
