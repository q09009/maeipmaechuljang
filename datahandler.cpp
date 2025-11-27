#include "datahandler.h"
#include <QDebug>
#include <QVariant>
#include <QString>
#include <vector>

DataHandler::DataHandler(QObject *parent) : QObject(parent)
{
    // 생성자에서는 초기화 작업만 수행합니다.
    // 엑셀 로드는 QML에서 loadExcelData()가 호출될 때 진행됩니다.
}

// ----------------------------------------------------
// 엑셀 데이터 로드 함수 (QML이 호출)
// ----------------------------------------------------
bool DataHandler::loadExcelData(const QString &filePath)
{
    // C++ 표준 문자열 변환 없이 바로 QString으로 경로 지정
    QXlsx::Document doc(filePath);

    // 1. 파일 로드 확인
    if (!doc.load()) {
        qWarning() << "경로에서 엑셀 파일 로드 실패:" << filePath;
        return false;
    }

    // 기존 데이터 초기화
    dataName.clear();
    dataProduct.clear();
    dataPSize.clear();
    dataPPrice.clear();

    // 2. 실제 데이터 읽기 함수 호출
    readDataFromExcel(doc);

    qDebug() << "엑셀 데이터 로드 성공. 업체 수:" << dataName.size();
    return true;
}

int maxNameRow;
int maxProductRow;
// ----------------------------------------------------
// 내부 데이터 읽기 로직 (업체명, 상품명)
// ----------------------------------------------------
void DataHandler::readDataFromExcel(QXlsx::Document &doc)
{
    int row = 2; // 데이터가 2행부터 시작한다고 가정
    int col_name = 1; // 1열: 업체명
    int col_product = 2; // 2열: 상품명
    int col_size = 3; // 3열: 규격
    int col_price = 4; // 4열: 단가

    while (true) {
        QVariant supplierVar = doc.read(row, col_name);
        QVariant productVar = doc.read(row, col_product);
        QVariant sizeVar = doc.read(row, col_size);
        QVariant priceVar = doc.read(row, col_price);

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

    QXlsx::Document doc("data/data.xlsx");

    int col_product = 2;
    int col_size = 3;
    int col_price = 4;

    doc.write(maxProductRow, col_product, product);
    doc.write(maxProductRow, col_size, size);
    doc.write(maxProductRow, col_price, price);
    maxProductRow++;

    doc.save();

    //readDataFromExcel(doc);
}

// ----------------------------------------------------
// 데이터 상품 쓰기 로직 (업체명)
// ----------------------------------------------------
void DataHandler::writeDataName(const QVariant &name, const QVariant &count) {

    QXlsx::Document doc("data/data.xlsx");

    doc.write(count.toInt() + 2, 1, name);

    doc.save();
}

// ----------------------------------------------------
// 레코드 쓰기 로직 (record.xlsx)
// ----------------------------------------------------
void DataHandler::writeExcelRecord(bool mae, const QVariant &date, const QVariant &supplier, const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &quantity) {

    QXlsx::Document doc("data/record.xlsx");

    int row = 2;
    while(true) {
        QVariant value = doc.read(row,1);

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

    doc.write(row, 1, mae ? "매입":"매출");
    doc.write(row, 2, dateOnly);
    doc.write(row, 3, supplier);
    doc.write(row, 4, product);
    doc.write(row, 5, size);
    doc.write(row, 6, price.toInt());
    doc.write(row, 7, quantity.toInt());
    doc.write(row, 8, gongga);
    doc.write(row, 9, tax);
    doc.write(row, 10, gongga.toInt() + tax.toInt());


    doc.save();
}

// ----------------------------------------------------
// 레코드 쓰기(입금일, 입금액) 로직 (record.xlsx)
// ----------------------------------------------------
void DataHandler::writeRecordIp(const QVariant &date, const QVariant &amount, const QVariant &row) {

    QXlsx::Document doc("data/record.xlsx");

    int rownum = row.toInt();
    int IpAmount = amount.toInt();
    //Misu는 미수금액
    int Misu = doc.read(rownum, 10).toInt() - IpAmount;

    QString dateString = date.toString();

    QDate myDate = QDate::fromString(dateString, "yyyy-MM-dd");



    doc.write(rownum, 11, myDate);
    doc.write(rownum, 12, IpAmount);
    doc.write(rownum, 13, Misu);



    doc.save();
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
    resultMisu.clear();
    readResultRows.clear();

    //이제부터 읽어주자
    QXlsx::Document doc("data/record.xlsx");
    int row = 2; // 데이터가 2행부터 시작한다고 가정
    std::vector<int> resultRows;
    int col_gooboon = 1; // 1열: 구분
    int col_date = 2; // 2열: 거래일자
    int col_supplier = 3; // 3열: 거래처명
    int col_product = 4; // 4열: 상품명

    while (true) {
        //조건과 비교해볼 자료들
        QVariant gooboonVar = doc.read(row, col_gooboon);
        QVariant dateVar = doc.read(row, col_date);
        QDate dateQ = dateVar.toDate();
        QVariant supplierVar = doc.read(row, col_supplier);
        QVariant productVar = doc.read(row, col_product);

        // A열(구분)이 빈 셀이면 반복문 종료
        if (!gooboonVar.isValid()) {
            break;
        }


        if(mae == true) {
            if(supplierVar == supplier && productVar == product && gooboonVar == "매입" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                resultRows.push_back(row);
            }
        }
        else if(mae == false) {
            if(supplierVar == supplier && productVar == product && gooboonVar == "매출" && dateQ>=startDate.toDate() && dateQ<=endDate.toDate()) {
                resultRows.push_back(row);
            }
        }

        row++;
    }
    if(resultRows.empty()) {
        return false;
    }

    for(int i=0;i<resultRows.size();i++) {
        //resultRows가 조건에 맞는 row니까 거기에 맞는 값들 다 담기
        QVariant resultGooboonVar = doc.read(resultRows[i], col_gooboon);
        QVariant resultDateVar = doc.read(resultRows[i], col_date);
        QDate resultDateQ = resultDateVar.toDate();
        QVariant resultSupVar = doc.read(resultRows[i], col_supplier);
        QVariant resultProVar = doc.read(resultRows[i], col_product);
        QVariant resultSizeVar = doc.read(resultRows[i], 5);
        QVariant resultPriceVar = doc.read(resultRows[i], 6);
        QVariant resultQuanVar = doc.read(resultRows[i], 7);
        QVariant resultGonggaVar = doc.read(resultRows[i], 8);
        QVariant resultBugaVar = doc.read(resultRows[i], 9);
        QVariant resultHapVar = doc.read(resultRows[i], 10);
        QVariant resultIpdateVar = doc.read(resultRows[i], 11);
        QDate resultIpdateQ = resultIpdateVar.toDate();
        QVariant resultIpAmountVar = doc.read(resultRows[i], 12);
        QVariant resultMisuVar = doc.read(resultRows[i], 13);
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
        resultIpdate.append(resultIpdateQ.toString("yyyy-MM-dd"));
        resultIpAmount.append(resultIpAmountVar.toInt());
        resultMisu.append(resultMisuVar.toInt());
        //조건에 맞는 Row들을 따로 저장해둠, 나중에 입금일, 입금액, 미수금액 처리하기 위함
        readResultRows.append(resultRows[i]);
    }
    return true;


    //maxProductRow = row;
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

QList<int> DataHandler::getReadResultRows() const
{
    return readResultRows;
}
