#ifndef SQLDATAHANDLER_H
#define SQLDATAHANDLER_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QDir>
#include <QStandardPaths>



class SqlHandler : public QObject {
    Q_OBJECT
public:
    explicit SqlHandler(QObject *parent = nullptr);
    ~SqlHandler();

    Q_INVOKABLE void initDB();             // DB 연결 및 테이블 생성
    void syncExcelToSql(const QList<QStringList>& dataList);      //엑셀에서 sql로 옮겨오는 함수
    void syncExcelToSqlData(const QVariantList& customers, const QList<QStringList>& items);      //data 파일엑셀에서 sql로 옮겨오는함수


    QList<QStringList> readAllSqlRecord();
    QList<QStringList> readAllSqlItem();
    QVariantList readAllSqlCustomer();

    //qt에서 항목 추가하거나 변경사항 있을때 불러줄 함수
    Q_INVOKABLE void refreshData();


    // sql에서 읽은 업체명 리스트를 반환
    Q_INVOKABLE QVariantList getDataName() const;

    // // sql에서 읽은 상품명 리스트를 반환
    Q_INVOKABLE QVariantList getDataProduct() const;

    // QML에서 data sql에 업체명 추가
    Q_INVOKABLE void writeDataName(const QVariant &name);

    // QML에서 data sql에 상품 추가
    Q_INVOKABLE void writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price);


    // record파일에 작성(미수금 제외)
    Q_INVOKABLE void writeExcelRecord(const bool &mae, const QVariant &date, const QVariant &supplier, const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &quantity, const bool &tax);

    // record파일에 작성(입금일, 입금액) 3개
    Q_INVOKABLE void writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2, const QVariant &amount2, const QVariant &date3, const QVariant &amount3, const QVariant &row);

    // 일괄 입금처리에 사용될 함수, searchedResult에서 값 읽어서 추가만 해주자
    Q_INVOKABLE void writeRecordIlgwalIpgeum(const QVariant &date, const QVariant &amount);

    // record파일에서 불러옴
    Q_INVOKABLE bool readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae, const QVariant &supplier, const QVariant &product);
    Q_INVOKABLE QVariantList getSearchedResult() const;
    //Q_INVOKABLE QVariantList getSearchedSum() const;
    Q_INVOKABLE QVariant getAmountSum() const;
    Q_INVOKABLE QVariant getGonggaSum() const;
    Q_INVOKABLE QVariant getBugaSum() const;
    Q_INVOKABLE QVariant getHapgyeSum() const;
    Q_INVOKABLE QVariant getIpamountSum() const;
    Q_INVOKABLE QVariant getMisuSum() const;
    Q_INVOKABLE QVariant getMijiSum() const;
    Q_INVOKABLE int getGaesoo() const;

    //월별통계 구하는 함수
    Q_INVOKABLE void monthTotalReady(const QVariant &year, const QVariant &gb, const QVariant &supplier, const QVariant &product);
    Q_INVOKABLE QVariantList getMonthTotal() const;
    Q_INVOKABLE QVariantList getBungiTotal() const;
    Q_INVOKABLE QVariantList getBangiTotal() const;
    //선택된 레코드 삭제하는 함수
    Q_INVOKABLE void deleteRecord(const QVariant &row);

    //이미 있는 data 수정하는함수들, 업체명이나 상품 정보들
    Q_INVOKABLE void editDataSupplier(const QVariant &name, const QVariant &count);
    Q_INVOKABLE void editDataProduct(const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &count);
    Q_INVOKABLE void deleteDataSupplier(const QVariant &count);
    Q_INVOKABLE void deleteDataProduct(const QVariant &count);

    // 월별 마감 엑셀 만드는데 보내줄것들
    QList<QStringList> readMonthlySql(const QVariant &year, const QVariant &month);

    // 백업 실행하는 함수
    bool backupDB();

    void cleanOldBackups();

private:
    QSqlDatabase m_db;

    //initDB할때 쓸 불러오는 함수들
    void initData();

    void calcSearchedSum(const QVariant &startDate, const QVariant &endDate, bool mae, const QVariant &supplier, const QVariant &product);

    void calcMonthTotal(QString queryStr);
    void calcBungiTotal(QString queryStr);
    void calcBangiTotal(QString queryStr);

    // C++ 내부에서 사용할 리스트 변수들
    QVariantList dataName;
    QVariantList dataProduct;
    QList<int> dataBalance;

    //검색결과들
    QVariantList searchedResult;

    QVariant amountSum;
    QVariant gonggaSum;
    QVariant bugaSum;
    QVariant hapgyeSum;
    QVariant ipamountSum;
    QVariant misuSum;
    QVariant mijiSum;
    int gaesoo;

    //월별통계용
    QVariantList monthTotal;
    QVariantList bungiTotal;
    QVariantList bangiTotal;

};

#endif // SQLDATAHANDLER_H
