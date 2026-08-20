#ifndef SQLDATAHANDLER_H
#define SQLDATAHANDLER_H

#include <QObject>
#include <QString>
#include <QVariantMap>
#include <memory>

#include "isqlstorage.h"

class SqlHandler : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool transferInProgress READ transferInProgress NOTIFY transferInProgressChanged)
public:
    enum class DbMode {
        Sqlite = 0,
        Api = 2
    };
    Q_ENUM(DbMode)

    enum class TransferDirection {
        SqliteToApi = 0,
        ApiToSqlite = 1
    };
    Q_ENUM(TransferDirection)

    explicit SqlHandler(QObject *parent = nullptr);
    ~SqlHandler();

    Q_INVOKABLE void setDbMode(DbMode mode, const QString &baseUrl = QString(),
                               const QString &apiKey = QString());

    Q_INVOKABLE QVariantMap getDbConfig() const;



    Q_INVOKABLE bool initDB();
    Q_INVOKABLE QString lastError() const;
    Q_INVOKABLE void startDataTransfer(TransferDirection direction);
    bool transferInProgress() const { return m_transferInProgress; }
    void syncExcelToSql(const QList<QStringList> &dataList);
    void syncExcelToSqlData(const QVariantList &customers, const QList<QStringList> &items);

    QList<QStringList> readAllSqlRecord();
    QList<QStringList> readAllSqlItem();
    QVariantList readAllSqlCustomer();

    Q_INVOKABLE void refreshData();
    Q_INVOKABLE QVariantList getDataName() const;
    Q_INVOKABLE QVariantList getDataProduct() const;

    Q_INVOKABLE void writeDataName(const QVariant &name);
    Q_INVOKABLE void writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price);

    Q_INVOKABLE void writeExcelRecord(const bool &mae, const QVariant &date, const QVariant &supplier,
                                      const QVariant &product, const QVariant &size, const QVariant &price,
                                      const QVariant &quantity, const bool &tax);

    Q_INVOKABLE void writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2,
                                   const QVariant &amount2, const QVariant &date3, const QVariant &amount3,
                                   const QVariant &row);
    Q_INVOKABLE void writeRecordIpFull(const QVariant &trDate, const QVariant &date1, const QVariant &amount1,
                                       const QVariant &date2, const QVariant &amount2, const QVariant &date3,
                                       const QVariant &amount3, const QVariant &row);

    Q_INVOKABLE void writeRecordIlgwalIpgeum(const QVariant &date, const QVariant &amount);

    Q_INVOKABLE bool readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae,
                                     const QVariant &supplier, const QVariant &product);
    Q_INVOKABLE QVariantList getSearchedResult() const;
    Q_INVOKABLE QVariant getAmountSum() const;
    Q_INVOKABLE QVariant getGonggaSum() const;
    Q_INVOKABLE QVariant getBugaSum() const;
    Q_INVOKABLE QVariant getHapgyeSum() const;
    Q_INVOKABLE QVariant getIpamountSum() const;
    Q_INVOKABLE QVariant getMisuSum() const;
    Q_INVOKABLE QVariant getMijiSum() const;
    Q_INVOKABLE int getGaesoo() const;

    Q_INVOKABLE void monthTotalReady(const QVariant &year, const QVariant &gb,
                                     const QVariant &supplier, const QVariant &product);
    Q_INVOKABLE QVariantList getMonthTotal() const;
    Q_INVOKABLE QVariantList getBungiTotal() const;
    Q_INVOKABLE QVariantList getBangiTotal() const;

    Q_INVOKABLE void deleteRecord(const QVariant &row);
    Q_INVOKABLE void editDataSupplier(const QVariant &name, const QVariant &count);
    Q_INVOKABLE void editDataProduct(const QVariant &product, const QVariant &size,
                                     const QVariant &price, const QVariant &count);
    Q_INVOKABLE void deleteDataSupplier(const QVariant &count);
    Q_INVOKABLE void deleteDataProduct(const QVariant &count);

    QList<QStringList> readMonthlySql(const QVariant &year, const QVariant &month);

    bool backupDB();
    void cleanOldBackups();
    void cleanOldLogs();

signals:
    void transferInProgressChanged();
    void dataTransferFinished(const QVariantMap &result);

private:
    DbMode m_mode = DbMode::Sqlite;
    std::unique_ptr<ISqlStorage> m_storage;

    QString m_lastError;
    QString m_baseUrl = "http://127.0.0.1:8000";
    QString m_apiKey;
    bool m_transferInProgress = false;
};

#endif // SQLDATAHANDLER_H
