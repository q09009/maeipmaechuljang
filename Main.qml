import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "./qml"

ApplicationWindow {
    id: mainWindow
    visible: true
    width: 1280
    height: 720
    title: "매입매출장"

    // [Logic & Data] - 변경 없음

    property var supplierList: []
    property var productList: []
    property var supplierSearchList: []
    property var productSearchList


    property int amountSum: 0
    property int gonggaSum: 0
    property int bugaSum: 0
    property int hapgyeSum: 0
    property int ipamountSum: 0
    property int misuSum: 0
    property int mijiSum: 0
    property int gaesoo: 0
    property list<int> readRows
    property var combinedModel: []

    property int ipgeumAmount1: 0
    property int ipgeumAmount2: 0
    property int ipgeumAmount3: 0
    property var ipgeumDate1
    property var ipgeumDate2
    property var ipgeumDate3

    property bool searchedMae: true

    Component.onCompleted: {

        sqlData.initDB();
        supplierList = sqlData.getDataName();
        productList = sqlData.getDataProduct();
        supplierSearchList = ["전체", ...supplierList]
        productSearchList = [{"name": "전체"}, ...productList]

    }

    Component.onDestruction: {
        console.log("종료중...");

    }

    // [Menu] - 변경 없음
    menuBar: MenuBar {
        Menu {
            title: qsTr("데이터 관리")
            MenuItem { text: qsTr("업체 추가"); onTriggered: supplierAddPopup.open() }
            MenuItem { text: qsTr("상품 추가"); onTriggered: productAddPopup.open() }
            MenuItem { text: qsTr("업체 변경"); onTriggered: supplierEditPopup.open() }
            MenuItem { text: qsTr("상품 변경"); onTriggered: productEditPopup.open() }
            MenuItem { text: qsTr("업체 삭제"); onTriggered: supplierDeletePopup.open() }
            MenuItem { text: qsTr("상품 삭제"); onTriggered: productDeletePopup.open() }
            MenuItem { text: qsTr("엑셀 -> SQL"); onTriggered: recordEtoS.open() }
            MenuItem { text: qsTr("SQL -> 엑셀"); onTriggered: recordStoE.open() }
        }
        Menu {
            title: qsTr("통계")
            MenuItem { text: qsTr("월별통계"); onTriggered: monthStat.show() }
        }
        Menu {
            title: qsTr("도움말")
            //MenuItem { text: qsTr("최적화"); onTriggered: optimizationPopup.open() }
            MenuItem { text: qsTr("정보"); onTriggered: infoPopup.open() }
        }
    }

    // [Popups] - 변경 없음

    // Popup {
    //         id: loadingPopup
    //         anchors.centerIn: parent
    //         width: 150; height: 75
    //         modal: true // 팝업 뒤쪽 클릭 안 되게 막음
    //         focus: true
    //         closePolicy: Popup.NoAutoClose // 작업 끝날 때까지 안 닫히게 설정

    //         ColumnLayout {
    //             anchors.centerIn: parent
    //             spacing: 20

    //             Text {
    //                 text: "데이터를 불러오는 중..."
    //                 //anchors.horizontalCenter: parent
    //                 Layout.alignment: Qt.AlignHCenter
    //                 //Layout.alignment: Qt.AlignVCenter
    //             }
    //             //밑에거들 두개다 로딩하는거의 cpu 사용량이 너무 세서 로딩 애니메이션이 안나옴....
    //             // ProgressBar {
    //             //     indeterminate: true
    //             //     Layout.alignment: Qt.AlignHCenter

    //             // }

    //             // BusyIndicator {
    //             //     //anchors.horizontalCenter: parent
    //             //     running: true
    //             //     Layout.alignment: Qt.AlignHCenter
    //             // }
    //         }
    //     }

    //     // 2. C++ 시그널과 연결 (핵심!)
    //     Connections {
    //         target: excelData // main.cpp에서 등록한 객체 이름

    //         // 로딩 시작 시그널을 받으면 팝업 열기
    //         function onLoadingStarted() {
    //             loadingPopup.open()
    //         }

    //         // 로딩 완료 시그널을 받으면 팝업 닫기
    //         function onLoadingFinished() {
    //             loadingPopup.close()
    //             // 추가로 완료 알림 팝업을 띄우고 싶다면 여기에 작성
    //             bgLoadingFinished.open()
    //         }
    //     }

    //     // 완료 알림 팝업 (선택 사항)


    Popup {
        id: supplierEditPopup
        property var row
        width: 300; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: RowLayout {
            ColumnLayout {
                ComboBox {
                    id: supplierEditComboBox
                    Layout.preferredWidth: 175 // Layout 크기 제어
                    Layout.preferredHeight: 25
                    model: supplierList
                    currentIndex: 0
                    onActivated: (index) => {
                                     console.log("선택된 옵션:", supplierEditComboBox.currentText);
                                     supplierEditTextfield.text = supplierEditComboBox.currentText;
                                     supplierEditPopup.row = supplierEditComboBox.currentIndex + 1;
                                     console.log(supplierEditComboBox.currentIndex + 1);
                                 }

                    popup: Popup {
                        y: supplierEditComboBox.height - 1
                        width: supplierEditComboBox.width
                        height: Math.min(contentItem.implicitHeight, 600)
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: supplierEditComboBox.popup.visible ? supplierEditComboBox.delegateModel : null
                            currentIndex: supplierEditComboBox.highlightedIndex
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                        }
                    }
                }
                TextField { id: supplierEditTextfield; placeholderText: qsTr("수정할 업체명 입력"); Layout.preferredWidth: 175 }
            }
            Button {
                text: qsTr("수정")
                Layout.preferredWidth: 50
                onClicked: {
                    sqlData.editDataSupplier(supplierEditTextfield.text, supplierEditPopup.row);
                    console.log("수정 성공");
                    supplierList = [];
                    supplierSearchList = [];
                    sqlData.refreshData();
                    supplierList = sqlData.getDataName();
                    supplierSearchList = ["전체", ...supplierList];
                    supplierEditPopup.close();
                    editFinished.open();
                }
            }
            Button { text: qsTr("닫기"); onClicked: supplierEditPopup.close() }
        }
    }

    Popup {
        id: supplierDeletePopup
        width: 300; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: RowLayout {
            ColumnLayout {
                ComboBox {
                    id: supplierDeleteComboBox
                    Layout.preferredWidth: 175 // Layout 크기 제어
                    Layout.preferredHeight: 25
                    model: supplierList
                    currentIndex: 0
                    onActivated: (index) => {
                                     console.log("선택된 옵션:", supplierDeleteComboBox.currentText);
                                     console.log(supplierDeleteComboBox.currentIndex + 1);
                                 }

                    popup: Popup {
                        y: supplierDeleteComboBox.height - 1
                        width: supplierDeleteComboBox.width
                        height: Math.min(contentItem.implicitHeight, 600)
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: supplierDeleteComboBox.popup.visible ? supplierDeleteComboBox.delegateModel : null
                            currentIndex: supplierDeleteComboBox.highlightedIndex
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                        }
                    }
                }
            }
            Button {
                text: qsTr("삭제")
                Layout.preferredWidth: 50
                onClicked: {
                    sqlData.deleteDataSupplier(supplierDeleteComboBox.currentIndex + 1);
                    console.log("삭제 성공");
                    supplierList = [];
                    supplierSearchList = [];
                    sqlData.refreshData();
                    supplierList = sqlData.getDataName();
                    supplierSearchList = ["전체", ...supplierList];
                    supplierDeletePopup.close();
                    deleteFinished.open();
                }
            }
            Button { text: qsTr("닫기"); onClicked: supplierDeletePopup.close() }
        }
    }

    Popup {
        id: productDeletePopup
        width: 300; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: RowLayout {
            ColumnLayout {
                ComboBox {
                    id: productDeleteComboBox
                    Layout.preferredWidth: 175 // Layout 크기 제어
                    Layout.preferredHeight: 25
                    model: productList
                    textRole: "name"
                    currentIndex: 0
                    onActivated: (index) => {
                                     console.log("선택된 옵션:", productDeleteComboBox.currentText);
                                     console.log(productList[productDeleteComboBox.currentIndex].id);
                                 }

                    popup: Popup {
                        y: productDeleteComboBox.height - 1
                        width: productDeleteComboBox.width
                        height: Math.min(contentItem.implicitHeight, 600)
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: productDeleteComboBox.popup.visible ? productDeleteComboBox.delegateModel : null
                            currentIndex: productDeleteComboBox.highlightedIndex
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                        }
                    }
                }
            }
            Button {
                text: qsTr("삭제")
                Layout.preferredWidth: 50
                onClicked: {
                    sqlData.deleteDataProduct(productList[productDeleteComboBox.currentIndex].id);
                    console.log("삭제 성공");
                    productList = [];
                    productSearchList = [];
                    sqlData.refreshData();
                    productList = sqlData.getDataProduct();
                    productSearchList = [{"name": "전체"}, ...productList];
                    productDeletePopup.close();
                    deleteFinished.open();
                }
            }
            Button { text: qsTr("닫기"); onClicked: productDeletePopup.close() }
        }
    }

    Popup {
        id: supplierAddPopup
        width: 300; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: RowLayout {
            TextField { id: supplierAddTextfield; placeholderText: qsTr("업체명 입력"); Layout.fillWidth: true }
            Button {
                text: qsTr("입력")
                onClicked: {
                    sqlData.writeDataName(supplierAddTextfield.text);
                    supplierList = [];
                    supplierSearchList = [];
                    sqlData.refreshData();
                    supplierList = sqlData.getDataName();
                    supplierSearchList = ["전체", ...supplierList];
                    console.log("추가 성공");
                    supplierAddPopup.close();
                    recordAddedPopup.open();
                }
            }
            Button { text: qsTr("X"); onClicked: supplierAddPopup.close() }
        }
    }


    Popup {
        id: productEditPopup
        property var row
        width: 450; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: RowLayout {
            ColumnLayout {
                ComboBox {
                    id: productEditComboBox
                    Layout.preferredWidth: 300
                    Layout.preferredHeight: 25
                    model: productList
                    textRole: "name"
                    currentIndex: 0
                    onActivated: (index) => {
                                     console.log("선택된 옵션번호:", productEditComboBox.currentIndex);
                                     productEditName.text = productEditComboBox.currentText;
                                     productEditPopup.row = productList[productEditComboBox.currentIndex].id;
                                     console.log("row = ", productEditPopup.row);
                                     console.log("규격 = ", productList[productEditComboBox.currentIndex].spec);
                                     console.log("단가 = ", productList[productEditComboBox.currentIndex].price);
                                     productEditSize.text = productList[productEditComboBox.currentIndex].spec
                                     productEditPrice.text = productList[productEditComboBox.currentIndex].price
                                 }

                    popup: Popup {
                        y: productEditComboBox.height - 1
                        width: productEditComboBox.width
                        height: Math.min(contentItem.implicitHeight, 600)
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: productEditComboBox.popup.visible ? productEditComboBox.delegateModel : null
                            currentIndex: productEditComboBox.highlightedIndex
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                        }
                    }
                }
                RowLayout {
                    TextField { id: productEditName; placeholderText: qsTr("상품명"); Layout.fillWidth: true }
                    TextField { id: productEditSize; placeholderText: qsTr("규격"); Layout.preferredWidth: 80 }
                    TextField { id: productEditPrice; placeholderText: qsTr("단가"); Layout.preferredWidth: 100 }
                }
            }
            Button {
                text: qsTr("수정")
                onClicked:  {
                    sqlData.editDataProduct(productEditName.text, productEditSize.text, productEditPrice.text, productEditPopup.row);
                    productList = [];
                    productSearchList = [];
                    sqlData.refreshData();
                    productList = sqlData.getDataProduct();
                    productSearchList = [{"name": "전체"}, ...productList];
                    productEditPopup.close();
                    editFinished.open();
                }
            }
            Button { text: qsTr("닫기"); onClicked: productEditPopup.close() }
        }
    }

    Popup {
        id: productAddPopup
        width: 450; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: RowLayout {
            TextField { id: productAddName; placeholderText: qsTr("상품명"); Layout.fillWidth: true }
            TextField { id: productAddSize; placeholderText: qsTr("규격"); Layout.preferredWidth: 80 }
            TextField { id: productAddPrice; placeholderText: qsTr("단가"); Layout.preferredWidth: 100 }
            Button {
                text: qsTr("입력")
                onClicked:  {
                    sqlData.writeDataProduct(productAddName.text, productAddSize.text, productAddPrice.text);
                    mainWindow.productList.push(productAddName.text);

                    productList = [];
                    productSearchList = [];
                    sqlData.refreshData();
                    productList = sqlData.getDataProduct();
                    productSearchList = [{"name": "전체"}, ...productList];
                    console.log("추가 성공");
                    productAddPopup.close();
                    recordAddedPopup.open();
                }
            }
            Button { text: qsTr("X"); onClicked: productAddPopup.close() }
        }
    }
    MonthTotal {
        id: monthStat
    }

    Popup {
        id: infoPopup
        width: 200; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: ColumnLayout {
            Text {
                text: qsTr("매입매출장 프로그램")
                Layout.alignment: Qt.AlignLeft
            }
            Text {
                text: qsTr("버전 1.4")
                Layout.alignment: Qt.AlignLeft
            }

            Button { text: qsTr("닫기"); Layout.alignment: Qt.AlignRight; onClicked: infoPopup.close() }
        }
    }

    Popup {
        id: optimizationPopup
        property var row
        width: 200; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: ColumnLayout {
            Text {
                text: qsTr("record를 최적화 하시겠습니까?")
                Layout.alignment: Qt.AlignCenter
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Button {
                    text: qsTr("최적화")
                    onClicked: {
                        excelData.startOptimization();
                        optimizationFinished.open();
                        optimizationPopup.close();
                    }
                }
                Button {
                    text: qsTr("취소")
                    onClicked: optimizationPopup.close()
                }
            }
        }
    }

    component AskPopup : Popup {
        id: aPopup

        signal yesClicked()

        property alias labeltext: alabel.text
        property alias yestext: yes.text
        property alias notext: no.text
        width: 300; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: ColumnLayout {
            Text {
                id: alabel
                Layout.alignment: Qt.AlignCenter
            }
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Button {
                    id: yes
                    onClicked: {
                        aPopup.yesClicked();
                        aPopup.close();
                    }
                }
                Button {
                    id: no
                    onClicked: aPopup.close()
                }
            }
        }
    }

    AskPopup {
        id: deleteAskPopup
        property var row
        labeltext: "정말로 삭제하시겠습니까?"
        yestext: "삭제"
        notext: "취소"

        onYesClicked: {
            sqlData.deleteRecord(deleteAskPopup.row);
            deleteFinished.open();
        }
    }

    AskPopup {
        id: recordEtoS
        labeltext: "엑셀데이터들을 SQL로 옮기시겠습니까?"
        yestext: "예"
        notext: "아니오"

        onYesClicked: {
            sync.EtoS();
            supplierList = [];
            supplierSearchList = [];
            productList = [];
            productSearchList = [];
            sqlData.refreshData();
            productList = sqlData.getDataProduct();
            productSearchList = [{"name": "전체"}, ...productList];
            supplierList = sqlData.getDataName();
            supplierSearchList = ["전체", ...supplierList];
        }
    }

    AskPopup {
        id: recordStoE
        labeltext: "SQL데이터들을 엑셀로 옮기시겠습니까?"
        yestext: "예"
        notext: "아니오"

        onYesClicked: {
            sync.StoE();
            // supplierList = [];
            // supplierSearchList = [];
            // productList = [];
            // productSearchList = [];
            // sqlData.refreshData();
            // productList = sqlData.getDataProduct();
            // productSearchList = [{"name": "전체"}, ...productList];
            // supplierList = sqlData.getDataName();
            // supplierSearchList = ["전체", ...supplierList];
        }
    }

    component ResultPopup : Popup {
        id: rPopup
        property alias text: label.text
        property alias textColor: label.color
        width: 200; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: ColumnLayout {
            Text { id: label; font.bold: true; Layout.alignment: Qt.AlignHCenter }
            Button { text: qsTr("닫기"); Layout.alignment: Qt.AlignHCenter; onClicked: rPopup.close() }
        }
    }

    ResultPopup {
        id: optimizationFinished
        text: "최적화 완료"
    }

    ResultPopup {
        id: editFinished
        text: "수정 성공"
    }

    ResultPopup {
        id: noSelected
        text: "항목을 선택해주세요"
    }

    ResultPopup {
        id: deleteFinished
        text: "삭제 성공"
    }

    ResultPopup {
        id: bgLoadingFinished
        text: "백그라운드 로딩 성공"
    }

    ResultPopup {
        id: recordAddedPopup
        text: "추가 성공"
    }

    ResultPopup {
        id: searchFailed
        text: "조건에 맞는 값 없음"
    }

    Popup {
        id: ipgeumPopup


        width: 400; height: 120
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: ColumnLayout {
            RowLayout {
                Text { text: qsTr("입금일1") }
                TextField { id: ipgeumDate1; placeholderText: qsTr("YYYY-MM-DD"); text:mainWindow.ipgeumDate1==="" ? "" : mainWindow.ipgeumDate1 ; Layout.fillWidth: true }
                Text { text: qsTr("입금액1") }
                TextField { id: ipgeumAmount1; text:mainWindow.ipgeumAmount1 ; Layout.fillWidth: true }
            }
            RowLayout {
                Text { text: qsTr("입금일2") }
                TextField { id: ipgeumDate2; placeholderText: qsTr("YYYY-MM-DD"); text:mainWindow.ipgeumDate2==="" ? "" : mainWindow.ipgeumDate2 ; Layout.fillWidth: true }
                Text { text: qsTr("입금액2") }
                TextField { id: ipgeumAmount2; text:mainWindow.ipgeumAmount2 ; Layout.fillWidth: true }
            }
            RowLayout {
                Text { text: qsTr("입금일3") }
                TextField { id: ipgeumDate3; placeholderText: qsTr("YYYY-MM-DD"); text:mainWindow.ipgeumDate3==="" ? "" : mainWindow.ipgeumDate3 ; Layout.fillWidth: true }
                Text { text: qsTr("입금액3") }
                TextField { id: ipgeumAmount3; text:mainWindow.ipgeumAmount3 ; Layout.fillWidth: true }
            }
            RowLayout {
                Layout.alignment: Qt.AlignRight


                Button {
                    text: qsTr("입력")
                    onClicked: { sqlData.writeRecordIp(ipgeumDate1.text, ipgeumAmount1.text, ipgeumDate2.text, ipgeumAmount2.text, ipgeumDate3.text, ipgeumAmount3.text, searchResultList.selectedRow); ipgeumPopup.close(); }
                }
                Button { text: qsTr("취소"); onClicked: ipgeumPopup.close() }
            }
        }
    }

    Popup {
        id: ilgwalipgeumPopup
        width: 300; height: 100
        anchors.centerIn: parent
        modal: true
        closePolicy: Popup.CloseOnPressOutside
        contentItem: ColumnLayout {
            RowLayout {
                Text { text: qsTr("입금일") }
                TextField { id: ipgeumDate; placeholderText: qsTr("YYYY-MM-DD"); Layout.fillWidth: true }
                Text { text: qsTr("입금액") }
                TextField { id: ipgeumAmount; Layout.fillWidth: true }
            }
            RowLayout{
                Layout.alignment: Qt.AlignRight
                Button {
                    text: qsTr("입력")
                    onClicked: {
                        sqlData.writeRecordIlgwalIpgeum(ipgeumDate.text, ipgeumAmount.text)
                        recordAddedPopup.open()
                        ilgwalipgeumPopup.close()
                    }
                }
                Button { text: qsTr("X"); onClicked: ilgwalipgeumPopup.close() }
            }
        }
    }

    Popup { id: calendarPopup; width: 270; height: 300; anchors.centerIn: parent; modal: true; closePolicy: Popup.CloseOnPressOutside; contentItem: MyCalendar { anchors.fill: parent; calendarParent: 0 } }
    Popup { id: scalendarPopup1; width: 270; height: 300; anchors.centerIn: parent; modal: true; closePolicy: Popup.CloseOnPressOutside; contentItem: MyCalendar { anchors.fill: parent; calendarParent: 1 } }
    Popup { id: scalendarPopup2; width: 270; height: 300; anchors.centerIn: parent; modal: true; closePolicy: Popup.CloseOnPressOutside; contentItem: MyCalendar { anchors.fill: parent; calendarParent: 2 } }


    // [Main Layout]
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 15
        spacing: 15

        // ==================================================================
        // 1. 입력 영역
        // ==================================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#FFFFFF"
            border.color: "#DDDDDD"
            radius: 5

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Button {
                    id: maeip
                    property bool mae: true
                    text: qsTr(maeip.mae ? "매입" : "매출")
                    onClicked: maeip.mae = !maeip.mae
                    background: Rectangle { color: maeip.mae ? "#e1f5fe" : "#ffebee"; radius: 3; border.color: "#ccc" }
                }

                Button {
                    id: calendarButton
                    property date currentDate : new Date()
                    text: qsTr(`${currentDate.getFullYear()}-${currentDate.getMonth()+1}-${currentDate.getDate()}`)
                    onClicked: calendarPopup.open()
                }

                // 콤보박스 (사용자 디자인 복원 + Layout 적용)
                ComboBox {
                    id: supplierComboBox
                    Layout.preferredWidth: 200 // Layout 크기 제어
                    Layout.preferredHeight: 25
                    model: supplierList
                    currentIndex: 0
                    onActivated: (index) => { console.log("선택된 옵션:", supplierComboBox.currentText); }

                    popup: Popup {
                        y: supplierComboBox.height - 1
                        width: supplierComboBox.width
                        height: Math.min(contentItem.implicitHeight, 600)
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: supplierComboBox.popup.visible ? supplierComboBox.delegateModel : null
                            currentIndex: supplierComboBox.highlightedIndex
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                        }
                    }
                }

                ComboBox {
                    id: productComboBox
                    Layout.preferredWidth: 200
                    Layout.preferredHeight: 25
                    model: productList
                    textRole: "name"
                    currentIndex: 0
                    onActivated: (index) => { console.log("선택된 옵션번호:", productComboBox.currentIndex); }

                    popup: Popup {
                        y: productComboBox.height - 1
                        width: productComboBox.width
                        height: Math.min(contentItem.implicitHeight, 600)
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: productComboBox.popup.visible ? productComboBox.delegateModel : null
                            currentIndex: productComboBox.highlightedIndex
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                        }
                    }
                }

                TextField { id: textSize; readOnly: true; text: mainWindow.productList[productComboBox.currentIndex].spec; Layout.preferredWidth: 80; placeholderText: "규격" }
                TextField { id: textPrice; text: Number(mainWindow.productList[productComboBox.currentIndex].price).toFixed(0); Layout.preferredWidth: 100; placeholderText: "단가"; horizontalAlignment: Text.AlignRight }
                TextField { id: textAmount; text: "1"; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignRight }

                Button {
                    id: taxornot
                    property bool ta: true
                    text: qsTr(taxornot.ta ? "부가세 별도" : "부가세 없음")
                    onClicked: taxornot.ta = !taxornot.ta
                }

                TextField { id: textGongGa; readOnly: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; placeholderText: "공급가액";
                    text: {
                        var p=parseInt(textPrice.text), a=parseInt(textAmount.text);
                        if (isNaN(p) || isNaN(a)) return "0";

                        var result = p * a;
                        return result.toLocaleString(Qt.locale("ko_KR"), "f", 0).replace(/,/g, "");
                    }
                }
                TextField { id: textBuGa; readOnly: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; placeholderText: "부가세";
                    text: {
                        var g=parseInt(textGongGa.text);
                        if(!taxornot.ta) {
                            return "0";
                        }
                        if(isNaN(g)) return "0";
                        var result = g*0.1;

                        return result.toLocaleString(Qt.locale("ko_KR"), "f", 0).replace(/,/g, "");
                    } }
                TextField { id: textHapGye; readOnly: true; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight; placeholderText: "합계"; font.bold: true;
                    text: {
                        var g=parseInt(textGongGa.text), b=parseInt(textBuGa.text);

                        if (isNaN(g)) return "0";
                        var result = g+b;
                        return result.toLocaleString(Qt.locale("ko_KR"), "f", 0).replace(/,/g, "");
                    }
                }

                Button {
                    id: addRecord
                    text: qsTr("등록")
                    highlighted: true
                    onClicked: {
                        sqlData.writeExcelRecord(maeip.mae, calendarButton.currentDate, supplierComboBox.currentText, productComboBox.currentText, textSize.text, textPrice.text, textAmount.text, taxornot.ta)
                        recordAddedPopup.open()
                    }
                }
            }
        }

        // ==================================================================
        // 2. 검색 영역
        // ==================================================================
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#F5F7FA"
            border.color: "#DDDDDD"
            radius: 5

            RowLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10

                Text { text: "📅 기간"; font.bold: true }
                Button { id: searchCalendarFirst; property date currentDate : new Date(); text: qsTr(`${currentDate.getFullYear()}-${currentDate.getMonth()+1}-${currentDate.getDate()}`); onClicked: scalendarPopup1.open() }
                Text { text: "~" }
                Button { id: searchCalendarSecond; property date currentDate : new Date(); text: qsTr(`${currentDate.getFullYear()}-${currentDate.getMonth()+1}-${currentDate.getDate()}`); onClicked: scalendarPopup2.open() }

                Rectangle { width: 1; height: 20; color: "#ccc" }

                Button { id: searchMaeip; property bool mae: true; text: qsTr(searchMaeip.mae ? "매입" : "매출"); onClicked: searchMaeip.mae = !searchMaeip.mae }

                // 검색용 콤보박스 (사용자 디자인 복원 + Layout 적용)
                ComboBox {
                    id: searchSupplierComboBox
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 25
                    model: supplierSearchList
                    currentIndex: 0
                    onActivated: (index) => { console.log("선택된 옵션:", searchSupplierComboBox.currentText); }

                    popup: Popup {
                        y: searchSupplierComboBox.height - 1
                        width: searchSupplierComboBox.width
                        height: Math.min(contentItem.implicitHeight, 600)
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: searchSupplierComboBox.popup.visible ? searchSupplierComboBox.delegateModel : null
                            currentIndex: searchSupplierComboBox.highlightedIndex
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                        }
                    }
                }

                ComboBox {
                    id: searchProductComboBox
                    Layout.preferredWidth: 180
                    Layout.preferredHeight: 25
                    model: productSearchList
                    textRole: "name"
                    currentIndex: 0
                    onActivated: (index) => { console.log("선택된 옵션번호:", searchProductComboBox.currentIndex); }

                    popup: Popup {
                        y: searchProductComboBox.height - 1
                        width: searchProductComboBox.width
                        height: Math.min(contentItem.implicitHeight, 600)
                        padding: 1
                        contentItem: ListView {
                            clip: true
                            implicitHeight: contentHeight
                            model: searchProductComboBox.popup.visible ? searchProductComboBox.delegateModel : null
                            currentIndex: searchProductComboBox.highlightedIndex
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Button {
                    id: searchRecord
                    text: qsTr("🔍 검색")
                    highlighted: true
                    onClicked: {
                        mainWindow.searchedMae = searchMaeip.mae
                        if(sqlData.readRecordRange(searchCalendarFirst.currentDate, searchCalendarSecond.currentDate, searchMaeip.mae, searchSupplierComboBox.currentText, searchProductComboBox.currentText)) {
                            mainWindow.readRows = []; mainWindow.combinedModel = [];
                            mainWindow.amountSum = 0; mainWindow.gonggaSum = 0; mainWindow.bugaSum = 0;
                            mainWindow.hapgyeSum = 0; mainWindow.ipamountSum = 0; mainWindow.misuSum = 0;
                            mainWindow.mijiSum = 0;

                            combinedModel = sqlData.getSearchedResult();
                            mainWindow.amountSum = sqlData.getAmountSum();
                            mainWindow.gonggaSum = sqlData.getGonggaSum();
                            mainWindow.bugaSum = sqlData.getBugaSum();
                            mainWindow.hapgyeSum = sqlData.getHapgyeSum();
                            mainWindow.ipamountSum = sqlData.getIpamountSum();
                            mainWindow.misuSum = sqlData.getMisuSum();
                            mainWindow.mijiSum = sqlData.getMijiSum();
                            mainWindow.gaesoo = sqlData.getGaesoo();

                            // var recordGB = excelData.getResultGooboon();
                            // var recordDate = excelData.getResultDate();
                            // var recordSupplier = excelData.getResultSupplier();
                            // var recordProduct = excelData.getResultProduct();
                            // var recordSize = excelData.getResultSize();
                            // var recordPrice = excelData.getResultPrice();
                            // var recordQuantity = excelData.getResultQuantity();
                            // var recordGongga = excelData.getResultGongga();
                            // var recordBuga = excelData.getResultBuga();
                            // var recordHapgye = excelData.getResultHapgye();
                            // var recordIpdate = excelData.getResultIpdate();
                            // var recordIpAmount = excelData.getResultIpAmount();
                            // var recordMisu = excelData.getResultMisu();
                            // var recordMiji = excelData.getResultMiji();
                            // var recordRows = excelData.getReadResultRows();

                            // mainWindow.gaesoo = recordGB.length
                            // console.log(recordGB.length, "개 검색됨");

                            // for(let i=0;i<recordGB.length;i++) {
                            //     mainWindow.amountSum += recordQuantity[i];
                            //     mainWindow.gonggaSum += recordGongga[i];
                            //     mainWindow.bugaSum += recordBuga[i];
                            //     mainWindow.hapgyeSum += recordHapgye[i];
                            //     mainWindow.ipamountSum += recordIpAmount[i];
                            //     mainWindow.misuSum += recordMisu[i];
                            //     mainWindow.mijiSum += recordMiji[i];

                            //     if(mainWindow.searchedMae) {
                            //         mainWindow.combinedModel.push({
                            //             gb: recordGB[i], date: recordDate[i], supplier: recordSupplier[i],
                            //             product: recordProduct[i], size: recordSize[i], price: recordPrice[i],
                            //             quantity: recordQuantity[i], gongga: recordGongga[i], buga: recordBuga[i],
                            //             hapgye: recordHapgye[i], ipdate: recordIpdate[i], ipamount: recordIpAmount[i],
                            //             misu: recordMiji[i], rows: recordRows[i]
                            //         })
                            //     }
                            //     else {
                            //         mainWindow.combinedModel.push({
                            //             gb: recordGB[i], date: recordDate[i], supplier: recordSupplier[i],
                            //             product: recordProduct[i], size: recordSize[i], price: recordPrice[i],
                            //             quantity: recordQuantity[i], gongga: recordGongga[i], buga: recordBuga[i],
                            //             hapgye: recordHapgye[i], ipdate: recordIpdate[i], ipamount: recordIpAmount[i],
                            //             misu: recordMisu[i], rows: recordRows[i]
                            //         })
                            //     }

                            // }
                            // mainWindow.combinedModel = mainWindow.combinedModel
                        } else {
                            searchFailed.open();
                        }
                    }
                }
                Button { id: addIpgeumRecord; text: qsTr("💰 일괄입금처리");
                    onClicked: ilgwalipgeumPopup.open()
                }

                Button { id: adjIpgeumRecord; text: qsTr("입금수정");
                    onClicked: searchResultList.searchClicked ? ipgeumPopup.open() : noSelected.open()
                }
                Button {
                    id: deleteRecordButton
                    text: qsTr("❌ 항목 삭제")
                    onClicked: searchResultList.searchClicked ? deleteAskPopup.open() : noSelected.open()
                }
            }
        }

        // ==================================================================
        // 3. 요약 영역
        // ==================================================================
        Rectangle {
            id: searchSum
            Layout.fillWidth: true
            Layout.preferredHeight: 60
            color: "#E8F5E9"
            border.color: "#A5D6A7"
            radius: 5

            RowLayout {
                anchors.fill: parent
                anchors.margins: 15
                spacing: 30

                component SummaryItem: ColumnLayout {
                    property string title
                    property string value
                    property color valColor: "black"
                    spacing: 5
                    Text { text: title; font.pixelSize: 12; color: "#555" }
                    Text { text: value; font.pixelSize: 16; font.bold: true; color: valColor }
                }

                SummaryItem { title: "검색 건수"; value: mainWindow.gaesoo + " 건" }
                SummaryItem { title: "총 수량"; value: mainWindow.amountSum.toLocaleString(Qt.locale(), 'f', 0) }
                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#ccc" }
                SummaryItem { title: "총 공급가액"; value: mainWindow.gonggaSum.toLocaleString(Qt.locale(), 'f', 0); valColor: "blue" }
                SummaryItem { title: "총 부가세"; value: mainWindow.bugaSum.toLocaleString(Qt.locale(), 'f', 0) }
                SummaryItem { title: "총 합계금액"; value: mainWindow.hapgyeSum.toLocaleString(Qt.locale(), 'f', 0); valColor: "blue" }
                Rectangle { Layout.preferredWidth: 1; Layout.fillHeight: true; color: "#ccc" }
                SummaryItem { title: "총 입금액"; value: mainWindow.ipamountSum.toLocaleString(Qt.locale(), 'f', 0) }
                SummaryItem { title: `총 미${mainWindow.searchedMae ? "지급" : "수금"}액`; value: mainWindow.searchedMae ? mainWindow.mijiSum.toLocaleString(Qt.locale(), 'f', 0) : mainWindow.misuSum.toLocaleString(Qt.locale(), 'f', 0); valColor: "blue" }
                Item { Layout.fillWidth: true }
            }
        }

        // ==================================================================
        // 4. 리스트 영역 (ListView Area) - 디자인 및 스크롤바 수정
        // ==================================================================
        Rectangle {
            id: searchResult
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "white"
            border.color: "#ccc"
            radius: 2
            clip: true

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // [4-1] 헤더 (Header)
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 35
                    color: "#f0f0f0"
                    border.color: "#ddd"
                    border.width: 1

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 5; anchors.rightMargin: 5
                        spacing: 0

                        component HeaderText: Text {
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            font.bold: true
                            font.pixelSize: 13
                            color: "#333"
                        }

                        HeaderText { text: "구분"; Layout.preferredWidth: 50 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "일자"; Layout.preferredWidth: 90 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "거래처"; Layout.fillWidth: true; Layout.minimumWidth: 100 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "품명"; Layout.fillWidth: true; Layout.minimumWidth: 100 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "규격"; Layout.preferredWidth: 60 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "단가"; Layout.preferredWidth: 70 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "수량"; Layout.preferredWidth: 50 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "공급가"; Layout.preferredWidth: 80 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "부가세"; Layout.preferredWidth: 70 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "합계"; Layout.preferredWidth: 80 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "최근입금일"; Layout.preferredWidth: 90 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: "누적입금액"; Layout.preferredWidth: 70 }
                        Rectangle { width: 1; height: 20; color: "#ddd" }
                        HeaderText { text: `미${mainWindow.searchedMae ? "지급" : "수금"}액`; Layout.preferredWidth: 70 }

                        // 🌟 스크롤바 가림 방지용 빈 공간 (Spacer) 추가
                        Item { Layout.preferredWidth: 20 }
                    }
                }

                // [4-2] 데이터 리스트 (ListView)
                ListView {
                    id: searchResultList
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: mainWindow.combinedModel
                    // 스크롤바가 항상 보이도록 설정
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn; width: 15 }

                    property bool searchClicked: false
                    property int selectedRow

                    delegate: Rectangle {
                        width: parent.width
                        height: 30

                        // 🌟 요청 사항 반영: 파란 아웃라인 제거 -> 배경색 변경 (연한 파란색)
                        color: {
                            if (searchResultList.selectedRow === modelData.id && searchResultList.searchClicked) {
                                return "#E3F2FD" // 선택 시: 눈이 편안한 연한 하늘색
                            }
                            return index % 2 === 0 ? "#ffffff" : "#f9f9f9" // 기본: 흰색/회색 교차
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                searchResultList.selectedRow = modelData.id
                                deleteAskPopup.row = modelData.id
                                searchResultList.searchClicked = true
                                console.log("clicked:", modelData.id)
                                mainWindow.ipgeumAmount1 = 0;
                                mainWindow.ipgeumDate1 = "";
                                mainWindow.ipgeumAmount2 = 0;
                                mainWindow.ipgeumDate2 = "";
                                mainWindow.ipgeumAmount3 = 0;
                                mainWindow.ipgeumDate3 = "";
                                //excelData.readRecordIpGeum(searchResultList.selectedRow);
                                console.log("입금액1:", modelData.ipA1)
                                console.log("입금액2:", modelData.ipA2)
                                console.log("입금액3:", modelData.ipA3)
                                console.log("입금일1:", modelData.ipD1)
                                console.log("입금일2:", modelData.ipD2)
                                console.log("입금일3:", modelData.ipD3)
                                mainWindow.ipgeumAmount1 = modelData.ipA1;
                                mainWindow.ipgeumDate1 = modelData.ipD1;
                                mainWindow.ipgeumAmount2 = modelData.ipA2;
                                mainWindow.ipgeumDate2 = modelData.ipD2;
                                mainWindow.ipgeumAmount3 = modelData.ipA3;
                                mainWindow.ipgeumDate3 = modelData.ipD3;
                                console.log("ye")
                            }
                        }

                        RowLayout {
                            id: resultRowLayout
                            anchors.fill: parent
                            anchors.leftMargin: 5; anchors.rightMargin: 5
                            spacing: 0

                            property string latestDate: {
                                var dates = [modelData.ipD1, modelData.ipD2, modelData.ipD3]
                                var latest = ""

                                for (var i = 0; i < dates.length; i++) {
                                    var d = dates[i]
                                    if (!d || d === "") continue

                                    if (latest === "" || d > latest)
                                        latest = d
                                }
                                return latest
                            }

                            component ListText: Text {
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 13
                                elide: Text.ElideRight
                                leftPadding: 5; rightPadding: 5
                            }
                            component NumText: ListText { horizontalAlignment: Text.AlignRight }

                            function safeInt(v) {
                                var n = Number(v)
                                return n;


                            }

                            ListText { text: modelData.gb; Layout.preferredWidth: 50; horizontalAlignment: Text.AlignHCenter; color: text==="매입"?"red":"blue" }
                            ListText { text: modelData.date; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignHCenter }
                            ListText { text: modelData.supplier; Layout.fillWidth: true; Layout.minimumWidth: 100 }
                            ListText { text: modelData.product; Layout.fillWidth: true; Layout.minimumWidth: 100 }
                            ListText { text: modelData.size; Layout.preferredWidth: 60; horizontalAlignment: Text.AlignHCenter }

                            NumText { text: parseInt(modelData.price).toLocaleString(Qt.locale(),'f',0); Layout.preferredWidth: 70 }
                            NumText { text: modelData.quantity; Layout.preferredWidth: 50 }
                            NumText { text: parseInt(modelData.gongga).toLocaleString(Qt.locale(),'f',0); Layout.preferredWidth: 80 }
                            NumText { text: parseInt(modelData.buga).toLocaleString(Qt.locale(),'f',0); Layout.preferredWidth: 70 }
                            NumText { text: parseInt(modelData.hapgye).toLocaleString(Qt.locale(),'f',0); Layout.preferredWidth: 80; font.bold: true }
                            ListText { text: resultRowLayout.latestDate !== "" ? resultRowLayout.latestDate : "-"; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignHCenter }
                            NumText { text: resultRowLayout.safeInt(modelData.ipA1 + modelData.ipA2 + modelData.ipA3).toLocaleString(Qt.locale(),'f',0); Layout.preferredWidth: 70; color: "blue" }
                            NumText { text: mainWindow.searchedMae? resultRowLayout.safeInt(modelData.miji).toLocaleString(Qt.locale(),'f',0) : resultRowLayout.safeInt(modelData.misu).toLocaleString(Qt.locale(),'f',0); Layout.preferredWidth: 70; color: modelData.misu > 0 ? "red" : "black" }

                            // 🌟 스크롤바 가림 방지용 빈 공간 (데이터 행에도 추가)
                            Item { Layout.preferredWidth: 20 }
                        }
                        // 하단 구분선
                        Rectangle { width: parent.width; height: 1; color: "#eee"; anchors.bottom: parent.bottom }
                    }
                }
            }
        }
    }
}
