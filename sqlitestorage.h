#ifndef SQLITESTORAGE_H
#define SQLITESTORAGE_H

#include "isqlstorage.h"

#include <QSqlDatabase>
#include <QVariantList>

class SqliteStorage : public ISqlStorage {
public:
    explicit SqliteStorage(const QString &connectionName = "data_connection",
                           const QString &databasePath = QString());
    ~SqliteStorage() override;

    bool initDB() override;
    void syncExcelToSql(const QList<QStringList> &dataList) override;
    void syncExcelToSqlData(const QVariantList &customers, const QList<QStringList> &items) override;
    QList<QStringList> readAllSqlRecord() override;
    QList<QStringList> readAllSqlItem() override;
    QVariantList readAllSqlCustomer() override;
    void refreshData() override;

    QVariantList getDataName() const override;
    QVariantList getDataProduct() const override;

    void writeDataName(const QVariant &name) override;
    void writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price) override;

    void writeExcelRecord(const bool &mae, const QVariant &date, const QVariant &supplier,
                          const QVariant &product, const QVariant &size, const QVariant &price,
                          const QVariant &quantity, const bool &tax) override;

    void writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2,
                       const QVariant &amount2, const QVariant &date3, const QVariant &amount3,
                       const QVariant &row) override;
    void writeRecordIpFull(const QVariant &trDate, const QVariant &date1, const QVariant &amount1,
                           const QVariant &date2, const QVariant &amount2, const QVariant &date3,
                           const QVariant &amount3, const QVariant &row) override;

    void writeRecordIlgwalIpgeum(const QVariant &date, const QVariant &amount) override;

    bool readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae,
                         const QVariant &supplier, const QVariant &product) override;

    QVariantList getSearchedResult() const override;
    QVariant getAmountSum() const override;
    QVariant getGonggaSum() const override;
    QVariant getBugaSum() const override;
    QVariant getHapgyeSum() const override;
    QVariant getIpamountSum() const override;
    QVariant getMisuSum() const override;
    QVariant getMijiSum() const override;
    int getGaesoo() const override;

    void monthTotalReady(const QVariant &year, const QVariant &gb, const QVariant &supplier,
                         const QVariant &product) override;
    QVariantList getMonthTotal() const override;
    QVariantList getBungiTotal() const override;
    QVariantList getBangiTotal() const override;

    void deleteRecord(const QVariant &row) override;
    void editDataSupplier(const QVariant &name, const QVariant &count) override;
    void editDataProduct(const QVariant &product, const QVariant &size, const QVariant &price,
                         const QVariant &count) override;
    void deleteDataSupplier(const QVariant &count) override;
    void deleteDataProduct(const QVariant &count) override;

    QList<QStringList> readMonthlySql(const QVariant &year, const QVariant &month) override;

    QJsonObject exportTransferSnapshot(QString *error) override;
    bool createTransferBackup(QString *backupReference, QString *error) override;
    bool replaceTransferSnapshot(const QJsonObject &snapshot,
                                 QString *backupReference, QString *error) override;

    bool backupDB() override;
    void cleanOldBackups() override;
    void cleanOldLogs() override;

private:
    bool createBackupWithPrefix(const QString &prefix, QString *backupReference,
                                QString *error);
    void initData();
    void calcSearchedSum(const QVariant &startDate, const QVariant &endDate, bool mae,
                         const QVariant &supplier, const QVariant &product);
    void calcMonthTotal(QString queryStr);
    void calcBungiTotal(QString queryStr);
    void calcBangiTotal(QString queryStr);

    QString m_connectionName;
    QString m_databasePath;
    QSqlDatabase m_db;

    QVariantList dataName;
    QVariantList dataProduct;
    QList<int> dataBalance;

    QVariantList searchedResult;

    QVariant amountSum;
    QVariant gonggaSum;
    QVariant bugaSum;
    QVariant hapgyeSum;
    QVariant ipamountSum;
    QVariant misuSum;
    QVariant mijiSum;
    int gaesoo = 0;

    QVariantList monthTotal;
    QVariantList bungiTotal;
    QVariantList bangiTotal;
};

#endif // SQLITESTORAGE_H
