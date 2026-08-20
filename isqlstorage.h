#ifndef ISQLSTORAGE_H
#define ISQLSTORAGE_H

#include <QList>
#include <QStringList>
#include <QVariant>

class ISqlStorage {
public:
    virtual ~ISqlStorage() = default;

    virtual bool initDB() = 0;
    virtual void syncExcelToSql(const QList<QStringList> &dataList) = 0;
    virtual void syncExcelToSqlData(const QVariantList &customers, const QList<QStringList> &items) = 0;

    virtual QList<QStringList> readAllSqlRecord() = 0;
    virtual QList<QStringList> readAllSqlItem() = 0;
    virtual QVariantList readAllSqlCustomer() = 0;

    virtual void refreshData() = 0;

    virtual QVariantList getDataName() const = 0;
    virtual QVariantList getDataProduct() const = 0;

    virtual void writeDataName(const QVariant &name) = 0;
    virtual void writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price) = 0;

    virtual void writeExcelRecord(const bool &mae, const QVariant &date, const QVariant &supplier,
                                  const QVariant &product, const QVariant &size, const QVariant &price,
                                  const QVariant &quantity, const bool &tax) = 0;

    virtual void writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2,
                               const QVariant &amount2, const QVariant &date3, const QVariant &amount3,
                               const QVariant &row) = 0;
    virtual void writeRecordIpFull(const QVariant &trDate, const QVariant &date1, const QVariant &amount1,
                                   const QVariant &date2, const QVariant &amount2, const QVariant &date3,
                                   const QVariant &amount3, const QVariant &row) = 0;

    virtual void writeRecordIlgwalIpgeum(const QVariant &date, const QVariant &amount) = 0;

    virtual bool readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae,
                                 const QVariant &supplier, const QVariant &product) = 0;

    virtual QVariantList getSearchedResult() const = 0;
    virtual QVariant getAmountSum() const = 0;
    virtual QVariant getGonggaSum() const = 0;
    virtual QVariant getBugaSum() const = 0;
    virtual QVariant getHapgyeSum() const = 0;
    virtual QVariant getIpamountSum() const = 0;
    virtual QVariant getMisuSum() const = 0;
    virtual QVariant getMijiSum() const = 0;
    virtual int getGaesoo() const = 0;

    virtual void monthTotalReady(const QVariant &year, const QVariant &gb, const QVariant &supplier,
                                 const QVariant &product) = 0;
    virtual QVariantList getMonthTotal() const = 0;
    virtual QVariantList getBungiTotal() const = 0;
    virtual QVariantList getBangiTotal() const = 0;

    virtual void deleteRecord(const QVariant &row) = 0;
    virtual void editDataSupplier(const QVariant &name, const QVariant &count) = 0;
    virtual void editDataProduct(const QVariant &product, const QVariant &size, const QVariant &price,
                                 const QVariant &count) = 0;
    virtual void deleteDataSupplier(const QVariant &count) = 0;
    virtual void deleteDataProduct(const QVariant &count) = 0;

    virtual QList<QStringList> readMonthlySql(const QVariant &year, const QVariant &month) = 0;

    virtual bool backupDB() = 0;
    virtual void cleanOldBackups() = 0;
    virtual void cleanOldLogs() = 0;
};

#endif // ISQLSTORAGE_H
