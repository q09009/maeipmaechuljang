#include <QList>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QtConcurrent>

#include "xlsxdocument.h"
#include "xlsxcell.h"
#include "xlsxformat.h"
//#include "xlsxstyles_p.h"

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
    Q_INVOKABLE QVariantList getResultMiji() const;
    Q_INVOKABLE QVariantList getResultMisu() const;

    Q_INVOKABLE QList<int> getReadResultRows() const;

    Q_INVOKABLE void makeDataExcel();
    Q_INVOKABLE void makeRecordExcel();
    // QML에서 record 엑셀로
    // Q_INVOKABLE QVariantList sendRecord() const;

    // // QML에서 data엑셀에 업체명 추가
    Q_INVOKABLE void writeDataName(const QVariant &name, const QVariant &count);

    // QML에서 data엑셀에 상품 추가
    Q_INVOKABLE void writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price);


    // // record파일에 작성(미수금 제외)
    Q_INVOKABLE void writeExcelRecord(bool mae, const QVariant &date, const QVariant &supplier, const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &quantity);

    // record파일에 작성(입금일, 입금액) 3개
    Q_INVOKABLE void writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2, const QVariant &amount2, const QVariant &date3, const QVariant &amount3, const QVariant &row);

    // // record파일에서 불러옴
    Q_INVOKABLE bool readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae, const QVariant &supplier, const QVariant &product);

    // record파일에서 입금일, 입금액 불러오기
    Q_INVOKABLE void readRecordIpGeum(const QVariant &row);

    //입금일하고 입금액들 반환해줄것들
    Q_INVOKABLE QVariant getipDate1() const;
    Q_INVOKABLE QVariant getipAmount1() const;
    Q_INVOKABLE QVariant getipDate2() const;
    Q_INVOKABLE QVariant getipAmount2() const;
    Q_INVOKABLE QVariant getipDate3() const;
    Q_INVOKABLE QVariant getipAmount3() const;

    // // data파일에 작성
    // Q_INVOKABLE bool writeExcelData(const QString &filePath);

    // QML 호출 가능 함수: C++에서 엑셀 파일을 로드하도록 지시
    Q_INVOKABLE bool loadExcelData();

    //월별통계 구하는 함수, 저거를 바탕으로 엑셀에다가 sumproduct 써서 입력
    Q_INVOKABLE void getMonthTotal(const QVariant &year, const QVariant &gb, const QVariant &supplier, const QVariant &product);

    //선택된 레코드 삭제하는 함수
    Q_INVOKABLE void deleteRecord(const QVariant &row);

    //이미 있는 data 수정하는함수들, 업체명이나 상품 정보들
    Q_INVOKABLE void editDataSupplier(const QVariant &name, const QVariant &row);
    Q_INVOKABLE void editDataProduct(const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &row);

    Q_INVOKABLE int getDataSupRow(const QVariant &name);
    Q_INVOKABLE int getDataProRow(const QVariant &name);
    Q_INVOKABLE QVariant getDataSizeEdit();
    Q_INVOKABLE QVariant getDataPriceEdit();

    int getRecordRows();
    //월별통계 반환하는 함수, 입력된거를 읽어오는놈들
    Q_INVOKABLE QVariantList getMTAmount() const;
    Q_INVOKABLE QVariantList getMTGongga() const;
    Q_INVOKABLE QVariantList getMTBuga() const;
    Q_INVOKABLE QVariantList getMTHapgye() const;
    Q_INVOKABLE QVariantList getMTMisu() const;
    Q_INVOKABLE QVariantList getMTMiji() const;

    //로드 속도가 느릴때는 글씨체, 글씨크기가 다른게 있거나 빈 행이 있으면 속도가 확 느려짐, 그거 해결용
    Q_INVOKABLE void startOptimization();
    Q_INVOKABLE void deleteZeros();

    Q_INVOKABLE QVariant test();
    Q_INVOKABLE void loadExcelInBackground();

signals:
    //처음 로딩할때 좀 오래걸리니까 로딩화면 추가해줘야지
    void loadingStarted();
    void loadingFinished();
    void optStarted();
    void optFinished();



private:

    std::atomic<bool> m_isLoading{false}; // 여러 스레드에서 접근하므로 atomic이 안전합니다.
    //record하고 data를 가리키는 포인터
    QXlsx::Document *m_recordDoc = nullptr;
    QXlsx::Document *m_dataDoc = nullptr;
    //파일을 한번만 로드하도록 보장하는 내부함수
    void ensureRecordLoaded();
    void ensureDataLoaded();

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
    QList<QVariant> resultMiji;
    QList<QVariant> resultMisu;

    QList<int> readResultRows;

    QVariant ipDate1;
    QVariant ipAmount1;
    QVariant ipDate2;
    QVariant ipAmount2;
    QVariant ipDate3;
    QVariant ipAmount3;

    QList<int> mtAmount;
    QList<int> mtGongga;
    QList<int> mtBuga;
    QList<int> mtHapgye;
    QList<int> mtMisu;
    QList<int> mtMiji;



    // 엑셀 파일 로드 및 데이터 읽기 로직 (여기서 엑셀 파일을 읽습니다)
    void readDataFromExcel();



    int dataEditRow;



    // void writeDataName(QXlsx::Document &doc);

    // void readRecordFromExcel(QXlsx::Document &doc);

    // void writeRecordFromExcel(QXlsx::Document &doc);
};

#endif // DATAHANDLER_H
