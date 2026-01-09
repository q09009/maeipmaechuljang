#include "datahandler.h"
#include <QDebug>
#include <QVariant>
#include <QString>
#include <QDir>
#include <vector>
#include <algorithm>
#include <QtConcurrent>

DataHandler::DataHandler(QObject *parent) : QObject(parent)
{
    // 생성자에서는 초기화 작업만 수행합니다.
    // 엑셀 로드는 QML에서 loadExcelData()가 호출될 때 진행됩니다.
}

void DataHandler::ensureRecordLoaded() {
    if(!m_recordDoc) {
        //record파일이 로드 안됐으면 로드
        m_recordDoc = new QXlsx::Document("data/record.xlsx");
        if(!m_recordDoc->load()) {
            //record파일이 없으면 새로 생성
            makeRecordExcel();
            m_recordDoc = new QXlsx::Document("data/record.xlsx");
        }
    }
}

void DataHandler::ensureDataLoaded() {
    if(!m_dataDoc) {
        //data파일이 로드 안됐으면 로드
        m_dataDoc = new QXlsx::Document("data/data.xlsx");
        if(!m_dataDoc->load()) {
            //data 파일이 없으면 새로 생성
            makeDataExcel();
            m_dataDoc = new QXlsx::Document("data/data.xlsx");
        }
    }
}


void DataHandler::startOptimization() {
    ensureRecordLoaded();

    int rownum = 2;


    QXlsx::Format defaultFormat;
    defaultFormat.setFontSize(11);
    defaultFormat.setFontName("맑은 고딕");

    while(true) {
        QVariant var = m_recordDoc->read(rownum, 1);
        if(rownum > m_recordDoc->dimension().lastRow()) {

            break;
        }
        else if(!var.isValid()) {
            //만약 빈칸이면
            bool notEmpty = false;
            for(int i=1;i<=18;i++) {
                //그 행 싹다 빈칸인지 확인하고
                QVariant isEmpty = m_recordDoc->read(rownum, i);
                if(isEmpty.isValid()) {
                    notEmpty = true;
                }
            }
            if(notEmpty) {
                //다 빈칸이 아니면 구분칸에 확인만 넣어둠
                m_recordDoc->write(rownum, 1, "확인");
            }
            else {
                deleteRecord(rownum);
            }
        }
        else {
            for(int i=1;i<=18;i++) {
                QVariant ye = m_recordDoc->read(rownum, i);
                if(i >= 6) {
                    if(ye.typeId() == QMetaType::QDate) {
                        m_recordDoc->write(rownum, i, ye, defaultFormat);
                    }
                    else {
                        qDebug() << ye << "before";
                        qDebug() << ye.toDouble() << "after";
                        m_recordDoc->write(rownum, i, ye.toDouble(), defaultFormat);
                    }
                }
                m_recordDoc->write(rownum, i, ye, defaultFormat);
            }
        }
        rownum++;
    }
    m_recordDoc->save();
    qDebug() << "최적화 완료";
}

void DataHandler::deleteZeros() {
    ensureRecordLoaded();

    int row = 2;
    while(true) {
        QVariant check = m_recordDoc->read(row, 1);

        if(!check.isValid()) {
            break;
        }

        for(int i=11;i<=16;i++) {
            QVariant var = m_recordDoc->read(row, i);
            QString strVal = var.toString().trimmed();

            if(strVal == "0") {
                m_recordDoc->write(row, i, QVariant());
            }
        }
        row++;
    }
    m_recordDoc->save();
}

void DataHandler::loadExcelInBackground() {
    if (m_isLoading) return;

    m_isLoading = true;
    emit loadingStarted();

    // 람다 함수를 사용하여 백그라운드에서 실행
    QtConcurrent::run([this]() {
        qDebug() << "백그라운드에서 엑셀 로딩을 시작합니다...";
        auto start = std::chrono::high_resolution_clock::now();

        // 실제 오래 걸리는 작업들
        if (!m_dataDoc) {
            m_dataDoc = new QXlsx::Document("data/data.xlsx");
            m_dataDoc->load();
        }


        if (!m_recordDoc) {
            m_recordDoc = new QXlsx::Document("data/record.xlsx");
            m_recordDoc->load();
        }

        m_isLoading = false;
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        double seconds = diff.count();
        qDebug() << "백그라운드 로딩 완료!" << seconds << "초 걸림";
        qDebug() << "Last Row:" << m_recordDoc->dimension().lastRow();

        // 작업이 끝나면 신호를 보냅니다.
        emit loadingFinished();
    });


}

// ----------------------------------------------------
// 엑셀 데이터 로드 함수 (QML이 호출)
// ----------------------------------------------------
bool DataHandler::loadExcelData()
{
    // C++ 표준 문자열 변환 없이 바로 QString으로 경로 지정
    //QXlsx::Document doc(filePath);
    ensureDataLoaded();
    //ensureRecordLoaded();

    // 1. 파일 로드 확인
    if (!m_dataDoc->load()) {
        qWarning() << "경로에서 엑셀 파일 로드 실패";
        return false;
    }

    // 기존 데이터 초기화
    dataName.clear();
    dataProduct.clear();
    dataPSize.clear();
    dataPPrice.clear();

    // 2. 실제 데이터 읽기 함수 호출
    readDataFromExcel();

    qDebug() << "엑셀 데이터 로드 성공. 업체 수:" << dataName.size();
    return true;
}

void DataHandler::makeDataExcel() {
    QDir dir;
    QXlsx::Document data;
    data.write("A1", "거래처명");
    data.write("B1", "상품명");
    data.write("C1", "규격");
    data.write("D1", "단가");

    if (!dir.exists("data")) { // 폴더가 이미 있는지 확인
        if (dir.mkdir("data")) {
            qDebug() << "폴더 생성 성공!";
        } else {
            qDebug() << "폴더 생성 실패";
        }
    }

    data.saveAs("data/data.xlsx");
}

void DataHandler::makeRecordExcel() {
    QDir dir;

    QXlsx::Document record;
    record.write("A1", "구분");
    record.write("B1", "거래일자");
    record.write("C1", "거래처명");
    record.write("D1", "상품명");
    record.write("E1", "규격");
    record.write("F1", "단가");
    record.write("G1", "수량");
    record.write("H1", "공급가액");
    record.write("I1", "부가세액");
    record.write("J1", "합계금액");
    record.write("K1", "입금일1");
    record.write("L1", "입금액1");
    record.write("M1", "입금일2");
    record.write("N1", "입금액2");
    record.write("O1", "입금일3");
    record.write("P1", "입금액3");
    record.write("Q1", "미지급액");
    record.write("R1", "미수금액");
    if (!dir.exists("data")) { // 폴더가 이미 있는지 확인
        if (dir.mkdir("data")) {
            qDebug() << "폴더 생성 성공!";
        } else {
            qDebug() << "폴더 생성 실패";
        }
    }
    record.saveAs("data/record.xlsx");
}

int maxNameRow;
int maxProductRow;
// ----------------------------------------------------
// 내부 데이터 읽기 로직 (업체명, 상품명)
// ----------------------------------------------------
void DataHandler::readDataFromExcel()
{
    ensureDataLoaded();

    int row = 2; // 데이터가 2행부터 시작한다고 가정
    int col_name = 1; // 1열: 업체명
    int col_product = 2; // 2열: 상품명
    int col_size = 3; // 3열: 규격
    int col_price = 4; // 4열: 단가

    while (true) {
        QVariant supplierVar = m_dataDoc->read(row, col_name);
        QVariant productVar = m_dataDoc->read(row, col_product);
        QVariant sizeVar = m_dataDoc->read(row, col_size);
        QVariant priceVar = m_dataDoc->read(row, col_price);

        // A열(업체명)이 빈 셀이면 반복문 종료
        if (!productVar.isValid()) {
            break;
        }

        // QVariant를 QString으로 변환하여 QList<QString>에 저장
        dataName.append(supplierVar.toString());    //빈 셀이어도 일단 저장
        dataProduct.append(productVar.toString());
        dataPSize.append(sizeVar.toString());
        dataPPrice.append(priceVar.toInt());

        row++;
    }
    maxProductRow = row;
}
// ----------------------------------------------------
// 데이터 상품 쓰기 로직 (상품명, 규격, 단가)
// ----------------------------------------------------
void DataHandler::writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price) {

    //QXlsx::Document doc("data/data.xlsx");
    ensureDataLoaded();

    int col_product = 2;
    int col_size = 3;
    int col_price = 4;

    m_dataDoc->write(maxProductRow, col_product, product);
    m_dataDoc->write(maxProductRow, col_size, size);
    m_dataDoc->write(maxProductRow, col_price, price);
    maxProductRow++;

    m_dataDoc->save();

    //readDataFromExcel(doc);
}

// ----------------------------------------------------
// 데이터 상품 쓰기 로직 (업체명)
// ----------------------------------------------------
void DataHandler::writeDataName(const QVariant &name, const QVariant &count) {

    //QXlsx::Document doc("data/data.xlsx");
    ensureDataLoaded();

    m_dataDoc->write(count.toInt() + 2, 1, name);

    m_dataDoc->save();
}

// ----------------------------------------------------
// 레코드 쓰기 로직 (record.xlsx)
// ----------------------------------------------------
void DataHandler::writeExcelRecord(bool mae, const QVariant &date, const QVariant &supplier, const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &quantity) {

    //QXlsx::Document doc("data/record.xlsx");
    ensureRecordLoaded();

    int row = 2;
    while(true) {
        QVariant value = m_recordDoc->read(row,1);

        if(!value.isValid()) {
            break;
        }
        row++;
    }
    QVariant gongga = price.toInt() * quantity.toInt();
    QVariant tax = gongga.toInt()/10;
    // 1. QVariant를 QDateTime으로 변환 (엑셀은 보통 DateTime으로 저장됨)
    QDateTime dateTime = date.toDateTime();
    // 2. QDate 부분만 추출합니다.
    QDate dateOnly = dateTime.date();

    m_recordDoc->write(row, 1, mae ? "매입":"매출");
    m_recordDoc->write(row, 2, dateOnly);
    m_recordDoc->write(row, 3, supplier);
    m_recordDoc->write(row, 4, product);
    m_recordDoc->write(row, 5, size);
    m_recordDoc->write(row, 6, price.toDouble());       //toDouble로 해야 숫자형식으로 저장됨
    m_recordDoc->write(row, 7, quantity.toDouble());
    m_recordDoc->write(row, 8, gongga);
    m_recordDoc->write(row, 9, tax);
    m_recordDoc->write(row, 10, gongga.toDouble() + tax.toDouble());
    //매입이면 미지급액칸에, 매출이면 미수금액칸에
    m_recordDoc->write(row, mae ? 17 : 18, gongga.toDouble() + tax.toDouble());


    m_recordDoc->save();
}
// ----------------------------------------------------
// 레코드 입금일, 입금액 읽어오기 로직 (record.xlsx)
// ----------------------------------------------------
void DataHandler::readRecordIpGeum(const QVariant &row) {
    ipDate1.clear();
    ipAmount1.clear();
    ipDate2.clear();
    ipAmount2.clear();
    ipDate3.clear();
    ipAmount3.clear();

    //QXlsx::Document doc("data/record.xlsx");
    ensureRecordLoaded();
    int rownum = row.toInt();

    QDate date1 = m_recordDoc->read(rownum, 11).toDate();
    ipDate1 = date1.toString("yyyy-MM-dd");
    if(m_recordDoc->read(rownum, 12).isNull()) {
        ipAmount1 = 0;
    }
    else {
        ipAmount1 = m_recordDoc->read(rownum, 12);
    }
    QDate date2 = m_recordDoc->read(rownum, 13).toDate();
    ipDate2 = date2.toString("yyyy-MM-dd");
    //ipAmount2 = doc.read(rownum, 15);
    if(m_recordDoc->read(rownum, 14).isNull()) {
        ipAmount2 = 0;
    }
    else {
        ipAmount2 = m_recordDoc->read(rownum, 14);
    }
    QDate date3 = m_recordDoc->read(rownum, 15).toDate();
    ipDate3 = date3.toString("yyyy-MM-dd");
    //ipAmount3 = doc.read(rownum, 17);
    if(m_recordDoc->read(rownum, 16).isNull()) {
        ipAmount3 = 0;
    }
    else {
        ipAmount3 = m_recordDoc->read(rownum, 16);
    }
    qDebug() << "레코드 입금일, 입금액 불러옴";

}

// ----------------------------------------------------
// 레코드 쓰기(입금일, 입금액) 로직 (record.xlsx)
// ----------------------------------------------------
void DataHandler::writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2, const QVariant &amount2, const QVariant &date3, const QVariant &amount3, const QVariant &row) {

    //QXlsx::Document doc("data/record.xlsx");
    ensureRecordLoaded();

    int rownum = row.toInt();
    int IpAmount1 = amount1.toInt();
    int IpAmount2 = amount2.toInt();
    int IpAmount3 = amount3.toInt();

    //빈칸인지 확인, 1900-01-00입력되는거 방지용
    if(date1 != "") {
        QString dateString1 = date1.toString();

        QDate myDate1 = QDate::fromString(dateString1, "yyyy-MM-dd");
        m_recordDoc->write(rownum, 11, myDate1);
        m_recordDoc->write(rownum, 12, amount1.toDouble());
    }
    else {
        m_recordDoc->write(rownum, 11, QVariant());
        m_recordDoc->write(rownum, 12, QVariant());
    }

    if(date2 != "") {
        QString dateString2 = date2.toString();

        QDate myDate2 = QDate::fromString(dateString2, "yyyy-MM-dd");

        m_recordDoc->write(rownum, 13, myDate2);
        m_recordDoc->write(rownum, 14, amount2.toDouble());
    }
    else {
        m_recordDoc->write(rownum, 13, QVariant());
        m_recordDoc->write(rownum, 14, QVariant());
    }

    if(date3 != "") {
        QString dateString3 = date3.toString();

        QDate myDate3 = QDate::fromString(dateString3, "yyyy-MM-dd");

        m_recordDoc->write(rownum, 15, myDate3);
        m_recordDoc->write(rownum, 16, amount3.toDouble());
    }
    else {
        m_recordDoc->write(rownum, 15, QVariant());
        m_recordDoc->write(rownum, 16, QVariant());
    }


    //Misu는 미수금액
    double Misu = m_recordDoc->read(rownum, 10).toInt() - IpAmount1 - IpAmount2 - IpAmount3;

    QVariant mae = m_recordDoc->read(rownum, 1);
    if(mae == "매입") {
        m_recordDoc->write(rownum, 17, Misu);
    }
    else {
        m_recordDoc->write(rownum, 18, Misu);
    }

    m_recordDoc->save();
}

// ----------------------------------------------------
// 레코드 읽기 로직 (record.xlsx)
// ----------------------------------------------------
bool DataHandler::readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae, const QVariant &supplier, const QVariant &product)
{
    //기존 데이터들 초기화
    resultGooboon.clear();
    resultDate.clear();
    resultSupplier.clear();
    resultProduct.clear();
    resultSize.clear();
    resultPrice.clear();
    resultQuantity.clear();
    resultGongga.clear();
    resultBuga.clear();
    resultHapgye.clear();
    resultIpdate.clear();
    resultIpAmount.clear();
    resultMiji.clear();
    resultMisu.clear();
    readResultRows.clear();

    //이제부터 읽어주자
    //QXlsx::Document doc("data/record.xlsx");
    ensureRecordLoaded();
    int row = 2; // 데이터가 2행부터 시작한다고 가정
    std::vector<int> resultRows;
    int col_gooboon = 1; // 1열: 구분
    int col_date = 2; // 2열: 거래일자
    int col_supplier = 3; // 3열: 거래처명
    int col_product = 4; // 4열: 상품명

    while (true) {
        //조건과 비교해볼 자료들
        QVariant gooboonVar = m_recordDoc->read(row, col_gooboon);
        QVariant dateVar = m_recordDoc->read(row, col_date);
        QDate dateQ = dateVar.toDate();
        QVariant supplierVar = m_recordDoc->read(row, col_supplier);
        QVariant productVar = m_recordDoc->read(row, col_product);

        // A열(구분)이 빈 셀이면 반복문 종료
        if (!gooboonVar.isValid()) {
            break;
        }


        if(mae == true) {
            if(supplier == "전체") {
                if(product == "전체") {       //매입, supplier전체, product전체
                    if(gooboonVar == "매입" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                        resultRows.push_back(row);
                    }
                }
                else {              //매입, supplier 전체, product not 전체
                    if(productVar == product && gooboonVar == "매입" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                        resultRows.push_back(row);
                    }
                }
            }
            else if(supplier != "전체") {
                if(product == "전체") {       //매입, supplier not 전체, product전체
                    if(supplierVar == supplier && gooboonVar == "매입" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                        resultRows.push_back(row);
                    }
                }
                else {              //매입, supplier not  전체, product not 전체
                    if(supplierVar == supplier && productVar == product && gooboonVar == "매입" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                        resultRows.push_back(row);
                    }
                }
            }
        }
        else if(mae == false) {
            if(supplier == "전체") {
                if(product == "전체") {       //매출, supplier전체, product전체
                    if(gooboonVar == "매출" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                        resultRows.push_back(row);
                    }
                }
                else {              //매출, supplier 전체, product not 전체
                    if(productVar == product && gooboonVar == "매출" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                        resultRows.push_back(row);
                    }
                }
            }
            else if(supplier != "전체") {
                if(product == "전체") {       //매출, supplier not 전체, product전체
                    if(supplierVar == supplier && gooboonVar == "매출" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                        resultRows.push_back(row);
                    }
                }
                else {              //매출, supplier not  전체, product not 전체
                    if(supplierVar == supplier && productVar == product && gooboonVar == "매출" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                        resultRows.push_back(row);
                    }
                }
            }
        }

        row++;
    }
    if(resultRows.empty()) {
        return false;
    }

    for(int i=0;i<resultRows.size();i++) {
        //resultRows가 조건에 맞는 row니까 거기에 맞는 값들 다 담기
        QVariant resultGooboonVar = m_recordDoc->read(resultRows[i], col_gooboon);
        QVariant resultDateVar = m_recordDoc->read(resultRows[i], col_date);
        QDate resultDateQ = resultDateVar.toDate();
        QVariant resultSupVar = m_recordDoc->read(resultRows[i], col_supplier);
        QVariant resultProVar = m_recordDoc->read(resultRows[i], col_product);
        QVariant resultSizeVar = m_recordDoc->read(resultRows[i], 5);
        QVariant resultPriceVar = m_recordDoc->read(resultRows[i], 6);
        QVariant resultQuanVar = m_recordDoc->read(resultRows[i], 7);
        QVariant resultGonggaVar = m_recordDoc->read(resultRows[i], 8);
        QVariant resultBugaVar = m_recordDoc->read(resultRows[i], 9);
        QVariant resultHapVar = m_recordDoc->read(resultRows[i], 10);
        QVariant resultIpdate1Var = m_recordDoc->read(resultRows[i], 11);
        QDate resultIpdate1Q = resultIpdate1Var.toDate();
        QVariant resultIpdate2Var = m_recordDoc->read(resultRows[i], 13);
        QDate resultIpdate2Q = resultIpdate2Var.toDate();
        QVariant resultIpdate3Var = m_recordDoc->read(resultRows[i], 15);
        QDate resultIpdate3Q = resultIpdate3Var.toDate();
        QDate latest = std::max({resultIpdate1Q, resultIpdate2Q, resultIpdate3Q});
        QVariant resultIpAmount1Var = m_recordDoc->read(resultRows[i], 12);
        QVariant resultIpAmount2Var = m_recordDoc->read(resultRows[i], 14);
        QVariant resultIpAmount3Var = m_recordDoc->read(resultRows[i], 16);
        QVariant resultMijiVar = m_recordDoc->read(resultRows[i], 17);  //미지급액
        QVariant resultMisuVar = m_recordDoc->read(resultRows[i], 18);  //미수금액
        resultGooboon.append(resultGooboonVar.toString());
        resultDate.append(resultDateQ.toString("yyyy-MM-dd"));
        resultSupplier.append(resultSupVar.toString());
        resultProduct.append(resultProVar.toString());
        resultSize.append(resultSizeVar.toString());
        resultPrice.append(resultPriceVar.toInt());
        resultQuantity.append(resultQuanVar.toInt());
        resultGongga.append(resultGonggaVar.toInt());
        resultBuga.append(resultBugaVar.toInt());
        resultHapgye.append(resultHapVar.toInt());
        resultIpdate.append(latest.toString("yyyy-MM-dd"));
        resultIpAmount.append(resultIpAmount1Var.toInt()+resultIpAmount2Var.toInt()+resultIpAmount3Var.toInt());
        resultMiji.append(resultMijiVar.toInt());
        resultMisu.append(resultMisuVar.toInt());
        //조건에 맞는 Row들을 따로 저장해둠, 나중에 입금일, 입금액, 미수금액 처리하기 위함
        readResultRows.append(resultRows[i]);
    }
    return true;


    //maxProductRow = row;
}

void DataHandler::getMonthTotal(const QVariant &year, const QVariant &gb, const QVariant &supplier, const QVariant &product) {
    //QXlsx::Document doc("data/record.xlsx");
    ensureRecordLoaded();

    //기존 데이터들 초기화
    mtAmount.clear();
    mtGongga.clear();
    mtBuga.clear();
    mtHapgye.clear();
    mtMisu.clear();
    mtMiji.clear();
    for(int i=0;i<12;i++) {
        mtAmount.append(0);
        mtGongga.append(0);
        mtBuga.append(0);
        mtHapgye.append(0);
        mtMisu.append(0);
        mtMiji.append(0);
    }

    int row = 2; // 데이터가 2행부터 시작한다고 가정
    std::vector<int> mtRows;
    int col_gooboon = 1; // 1열: 구분
    int col_date = 2; // 2열: 거래일자
    int col_supplier = 3; // 3열: 거래처명
    int col_product = 4; // 4열: 상품명

    while (true) {
        //조건과 비교해볼 자료들
        QVariant gooboonVar = m_recordDoc->read(row, col_gooboon);
        QVariant dateVar = m_recordDoc->read(row, col_date);
        QDate dateQ = dateVar.toDate();
        QVariant supplierVar = m_recordDoc->read(row, col_supplier);
        QVariant productVar = m_recordDoc->read(row, col_product);

        // A열(구분)이 빈 셀이면 반복문 종료
        if (!gooboonVar.isValid()) {
            break;
        }


        if(gb == "매입") {
            if(supplier == "전체") {
                if(product == "전체") {       //매입, supplier전체, product전체
                    if(gooboonVar == "매입" && dateQ.year()==year.toInt()) {
                        mtRows.push_back(row);

                    }
                }
                else {              //매입, supplier 전체, product not 전체
                    if(productVar == product && gooboonVar == "매입" && dateQ.year()==year.toInt()) {
                        mtRows.push_back(row);

                    }
                }
            }
            else if(supplier != "전체") {
                if(product == "전체") {       //매입, supplier not 전체, product전체
                    if(supplierVar == supplier && gooboonVar == "매입" && dateQ.year()==year.toInt()) {
                        mtRows.push_back(row);

                    }
                }
                else {              //매입, supplier not  전체, product not 전체
                    if(supplierVar == supplier && productVar == product && gooboonVar == "매입" && dateQ.year()==year.toInt()) {
                        mtRows.push_back(row);

                    }
                }
            }
        }
        else if(gb == "매출") {
            if(supplier == "전체") {
                if(product == "전체") {       //매출, supplier전체, product전체
                    if(gooboonVar == "매출" && dateQ.year()==year.toInt()) {
                        mtRows.push_back(row);

                    }
                }
                else {              //매출, supplier 전체, product not 전체
                    if(productVar == product && gooboonVar == "매출" && dateQ.year()==year.toInt()) {
                        mtRows.push_back(row);

                    }
                }
            }
            else if(supplier != "전체") {
                if(product == "전체") {       //매출, supplier not 전체, product전체
                    if(supplierVar == supplier && gooboonVar == "매출" && dateQ.year()==year.toInt()) {
                        mtRows.push_back(row);

                    }
                }
                else {              //매출, supplier not  전체, product not 전체
                    if(supplierVar == supplier && productVar == product && gooboonVar == "매출" && dateQ.year()==year.toInt()) {
                        mtRows.push_back(row);

                    }
                }
            }
        }

        row++;
    }

    for(int i=0;i<mtRows.size();i++) {
        //resultRows가 조건에 맞는 row니까 거기에 맞는 값들 다 담기
        QVariant resultDateVar = m_recordDoc->read(mtRows[i], col_date);
        QDate resultDateQ = resultDateVar.toDate();
        int month = resultDateQ.month() - 1;
        //qDebug() << month;
        QVariant resultQuanVar = m_recordDoc->read(mtRows[i], 7);
        QVariant resultGonggaVar = m_recordDoc->read(mtRows[i], 8);
        QVariant resultBugaVar = m_recordDoc->read(mtRows[i], 9);
        QVariant resultHapVar = m_recordDoc->read(mtRows[i], 10);
        QVariant resultMijiVar = m_recordDoc->read(mtRows[i], 17);
        QVariant resultMisuVar = m_recordDoc->read(mtRows[i], 18);

        mtAmount[month] += resultQuanVar.toInt();
        mtGongga[month] += resultGonggaVar.toInt();
        mtBuga[month] += resultBugaVar.toInt();
        mtHapgye[month] += resultHapVar.toInt();
        mtMisu[month] += resultMisuVar.toInt();
        mtMiji[month] += resultMijiVar.toInt();

    }
}


int DataHandler::getRecordRows()
{
    QXlsx::Document doc("data/record.xlsx");

    int row = 2; // 데이터가 2행부터 시작한다고 가정


    while (true) {
        QVariant var = doc.read(row, 1);


        // A열(구분)이 빈 셀이면 반복문 종료
        if (!var.isValid()) {
            break;
        }
        row++;
    }
    return row;
}

QVariant DataHandler::test() {
    QXlsx::Document doc("data/record.xlsx");

    //doc.write(1, 1, "=5+4");
    //doc.save();
    //auto cell = doc.cellAt(1, 1);
    QVariant val = doc.read(2, 2);
    qDebug() << val;

    return val;
}


void DataHandler::editDataSupplier(const QVariant &name, const QVariant &row) {
    ensureDataLoaded();

    int rownum = row.toInt();
    m_dataDoc->write(rownum, 1, name);

    m_dataDoc->save();
}

void DataHandler::editDataProduct(const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &row) {
    ensureDataLoaded();

    int rownum = row.toInt();
    m_dataDoc->write(rownum, 2, product);
    m_dataDoc->write(rownum, 3, size);
    m_dataDoc->write(rownum, 4, price);

    m_dataDoc->save();
}

int DataHandler::getDataSupRow(const QVariant &name) {
    ensureDataLoaded();

    int row = 2;
    while (true) {
        QVariant supplierVar = m_dataDoc->read(row, 1);

        // A열(업체명)이 빈 셀이면 반복문 종료
        if (supplierVar == name) {
            break;
        }

        row++;
    }
    return row;
}

int DataHandler::getDataProRow(const QVariant &name) {
    ensureDataLoaded();

    int row = 2;
    while (true) {
        QVariant productVar = m_dataDoc->read(row, 2);
        // A열(업체명)이 빈 셀이면 반복문 종료
        if (productVar == name) {
            break;
        }
        //qDebug() << productVar;
        row++;
    }
    qDebug() << row;
    dataEditRow = row;
    return row;
}

QVariant DataHandler::getDataSizeEdit() {
    ensureDataLoaded();
    QVariant sizeVar = m_dataDoc->read(dataEditRow, 3);
    return sizeVar;
}

QVariant DataHandler::getDataPriceEdit() {
    ensureDataLoaded();
    QVariant priceVar = m_dataDoc->read(dataEditRow, 4);
    return priceVar;
}

void DataHandler::deleteRecord(const QVariant &row)
{
    ensureRecordLoaded();

    int rownum = row.toInt(); // 데이터가 2행부터 시작한다고 가정


    //여기서부터는 삭제한 줄 밑에를 보면서 밑에칸에서 한칸씩 위로 올려주는거임
    while (true) {
        QVariant gbVar = m_recordDoc->read(rownum+1, 1);
        QVariant dateVar = m_recordDoc->read(rownum+1, 2);
        QVariant supVar = m_recordDoc->read(rownum+1, 3);
        QVariant proVar = m_recordDoc->read(rownum+1, 4);
        QVariant sizeVar = m_recordDoc->read(rownum+1, 5);
        QVariant priceVar = m_recordDoc->read(rownum+1, 6);
        QVariant amountVar = m_recordDoc->read(rownum+1, 7);
        QVariant ggVar = m_recordDoc->read(rownum+1, 8);
        QVariant bgVar = m_recordDoc->read(rownum+1, 9);
        QVariant hgVar = m_recordDoc->read(rownum+1, 10);
        QVariant ipdate1Var = m_recordDoc->read(rownum+1, 11);
        QVariant ipam1Var = m_recordDoc->read(rownum+1, 12);
        QVariant mijiVar = m_recordDoc->read(rownum+1, 17);
        QVariant misuVar = m_recordDoc->read(rownum+1, 18);
        QVariant ipdate2Var = m_recordDoc->read(rownum+1, 13);
        QVariant ipam2Var = m_recordDoc->read(rownum+1, 14);
        QVariant ipdate3Var = m_recordDoc->read(rownum+1, 15);
        QVariant ipam3Var = m_recordDoc->read(rownum+1, 16);

        // 밑에 구분칸이 빈 셀이면 반복문 종료
        if (!gbVar.isValid()) {
            break;
        }

        //빈칸이 아니라면 이제 위로 올려주자 ㅋㅋ
        m_recordDoc->write(rownum, 1, gbVar);
        m_recordDoc->write(rownum, 2, dateVar);
        m_recordDoc->write(rownum, 3, supVar);
        m_recordDoc->write(rownum, 4, proVar);
        m_recordDoc->write(rownum, 5, sizeVar);
        m_recordDoc->write(rownum, 6, priceVar.toDouble());
        m_recordDoc->write(rownum, 7, amountVar.toDouble());
        m_recordDoc->write(rownum, 8, ggVar.toDouble());
        m_recordDoc->write(rownum, 9, bgVar.toDouble());
        m_recordDoc->write(rownum, 10, hgVar.toDouble());
        m_recordDoc->write(rownum, 11, ipdate1Var);
        m_recordDoc->write(rownum, 12, ipam1Var.toDouble());
        m_recordDoc->write(rownum, 13, ipdate2Var);
        m_recordDoc->write(rownum, 14, ipam2Var.toDouble());
        m_recordDoc->write(rownum, 15, ipdate3Var);
        m_recordDoc->write(rownum, 16, ipam3Var.toDouble());
        m_recordDoc->write(rownum, 17, mijiVar.toDouble());
        m_recordDoc->write(rownum, 18, misuVar.toDouble());


        //다옮겼으면 이제 밑에로 한칸
        rownum++;
    }
    //이제 밑에꺼 삭제
    for(int i=0;i<18;i++) {
        m_recordDoc->write(rownum, i+1, QVariant());
    }
    m_recordDoc->save();

}
// ----------------------------------------------------
// QML 데이터 반환 함수
// ----------------------------------------------------
QVariantList DataHandler::getDataName() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : dataName) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getDataProduct() const
{
    QVariantList list;
    for (const QVariant &name : dataProduct) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getDataSize() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &size : dataPSize) {
        list.append(QVariant::fromValue(size));
    }
    return list;
}

QVariantList DataHandler::getDataPrice() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &price : dataPPrice) {
        list.append(QVariant::fromValue(price));
    }
    return list;
}

QVariantList DataHandler::getResultGooboon() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultGooboon) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultDate() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultDate) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultSupplier() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultSupplier) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultProduct() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultProduct) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultSize() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultSize) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultPrice() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultPrice) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultQuantity() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultQuantity) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultGongga() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultGongga) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultBuga() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultBuga) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultHapgye() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultHapgye) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultIpdate() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultIpdate) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultIpAmount() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultIpAmount) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultMisu() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultMisu) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getResultMiji() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : resultMiji) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QList<int> DataHandler::getReadResultRows() const
{
    return readResultRows;
}

QVariant DataHandler::getipAmount1() const
{
    return ipAmount1;
}

QVariant DataHandler::getipDate1() const
{
    return ipDate1;
}
QVariant DataHandler::getipAmount2() const
{
    return ipAmount2;
}

QVariant DataHandler::getipDate2() const
{
    return ipDate2;
}
QVariant DataHandler::getipAmount3() const
{
    return ipAmount3;
}

QVariant DataHandler::getipDate3() const
{
    return ipDate3;
}

QVariantList DataHandler::getMTAmount() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : mtAmount) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getMTGongga() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : mtGongga) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getMTBuga() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : mtBuga) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getMTHapgye() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : mtHapgye) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getMTMisu() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : mtMisu) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}

QVariantList DataHandler::getMTMiji() const
{
    QVariantList list;
    // QList<QVariant>의 데이터를 QVariantList로 변환하여 QML로 전달
    for (const QVariant &name : mtMiji) {
        list.append(QVariant::fromValue(name));
    }
    return list;
}
