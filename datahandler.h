#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>

#include "xlsxdocument.h"

#ifndef DATAHANDLER_H
#define DATAHANDLER_H

class DataHandler : public QObject
{
    Q_OBJECT
public:
    explicit DataHandler(QObject *parent = nullptr);

    // 엑셀에서 읽은 업체명 리스트를 반환
    Q_INVOKABLE QVariantList getDataName() const;

    // 엑셀에서 읽은 상품명 리스트를 반환
    Q_INVOKABLE QVariantList getDataProduct() const;

    // 엑셀에서 읽은 상품 사이즈 반환
    Q_INVOKABLE QVariantList getDataSize() const;

    // 엑셀에서 읽은 상품 가격 반환
    Q_INVOKABLE QVariantList getDataPrice() const;

    // record에서 읽어온 것들;; 와이리많노
    Q_INVOKABLE QVariantList getResultGooboon() const;
    Q_INVOKABLE QVariantList getResultDate() const;
    Q_INVOKABLE QVariantList getResultSupplier() const;
    Q_INVOKABLE QVariantList getResultProduct() const;
    Q_INVOKABLE QVariantList getResultSize() const;
    Q_INVOKABLE QVariantList getResultPrice() const;
    Q_INVOKABLE QVariantList getResultQuantity() const;
    Q_INVOKABLE QVariantList getResultGongga() const;
    Q_INVOKABLE QVariantList getResultBuga() const;
    Q_INVOKABLE QVariantList getResultHapgye() const;
    Q_INVOKABLE QVariantList getResultIpdate() const;
    Q_INVOKABLE QVariantList getResultIpAmount() const;
    Q_INVOKABLE QVariantList getResultMisu() const;

    Q_INVOKABLE QList<int> getReadResultRows() const;

    // QML에서 record 엑셀로
    // Q_INVOKABLE QVariantList sendRecord() const;

    // // QML에서 data엑셀에 업체명 추가
    Q_INVOKABLE void writeDataName(const QVariant &name, const QVariant &count);

    // QML에서 data엑셀에 상품 추가
    Q_INVOKABLE void writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price);


    // // record파일에 작성(미수금 제외)
    Q_INVOKABLE void writeExcelRecord(bool mae, const QVariant &date, const QVariant &supplier, const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &quantity);

    // record파일에 작성(입금일, 입금액)
    Q_INVOKABLE void writeRecordIp(const QVariant &date, const QVariant &amount, const QVariant &row);

    // // record파일에서 불러옴
    Q_INVOKABLE bool readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae, const QVariant &supplier, const QVariant &product);

    // // data파일에 작성
    // Q_INVOKABLE bool writeExcelData(const QString &filePath);

    // QML 호출 가능 함수: C++에서 엑셀 파일을 로드하도록 지시
    Q_INVOKABLE bool loadExcelData(const QString &filePath);

private:
    // C++ 내부에서 사용할 리스트 변수들
    QList<QVariant> dataName;
    QList<QVariant> dataProduct;
    QList<QVariant> dataPSize;
    QList<QVariant> dataPPrice;
    //QList<QVariant> sendingData;

    QList<QVariant> resultGooboon;
    QList<QVariant> resultDate;
    QList<QVariant> resultSupplier;
    QList<QVariant> resultProduct;
    QList<QVariant> resultSize;
    QList<QVariant> resultPrice;
    QList<QVariant> resultQuantity;
    QList<QVariant> resultGongga;
    QList<QVariant> resultBuga;
    QList<QVariant> resultHapgye;
    QList<QVariant> resultIpdate;
    QList<QVariant> resultIpAmount;
    QList<QVariant> resultMisu;

    QList<int> readResultRows;

    // 엑셀 파일 로드 및 데이터 읽기 로직 (여기서 엑셀 파일을 읽습니다)
    void readDataFromExcel(QXlsx::Document &doc);



    // void writeDataName(QXlsx::Document &doc);

    // void readRecordFromExcel(QXlsx::Document &doc);

    // void writeRecordFromExcel(QXlsx::Document &doc);
};

#endif // DATAHANDLER_H
