#ifndef APISTORAGE_H
#define APISTORAGE_H

#include "isqlstorage.h"

#include <QList>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QVariantList>

class ApiStorage : public ISqlStorage {
public:
    explicit ApiStorage(const QString &baseUrl = "http://127.0.0.1:8000",
                        const QString &apiKey = QString());

    QString lastError() const { return m_lastError; }

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

    void monthTotalReady(const QVariant &year, const QVariant &gb,
                         const QVariant &supplier, const QVariant &product) override;
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
    QByteArray syncRequest(const QByteArray &method, const QString &path,
                           const QByteArray &body = QByteArray(), int timeoutMs = 15000);
    QByteArray syncGet(const QString &path, int timeoutMs = 15000);
    QByteArray syncPost(const QString &path, const QByteArray &body,
                        int timeoutMs = 15000);
    QByteArray syncPut(const QString &path, const QByteArray &body);
    bool syncDelete(const QString &path);

    static QString isoDate(const QVariant &value);
    static void putNullableDate(QJsonObject &object, const QString &key, const QVariant &value);

    QString m_baseUrl;
    QString m_apiKey;
    QString m_lastError;
    bool m_lastRequestOk = false;
    QNetworkAccessManager m_nam;

    QVariantList m_dataName;
    QVariantList m_dataProduct;
    QList<int> m_customerIds;

    QVariantList m_searchedResult;
    QVariant m_amountSum;
    QVariant m_gonggaSum;
    QVariant m_bugaSum;
    QVariant m_hapgyeSum;
    QVariant m_ipamountSum;
    QVariant m_misuSum;
    QVariant m_mijiSum;
    int m_gaesoo = 0;

    QVariantList m_monthTotal;
    QVariantList m_bungiTotal;
    QVariantList m_bangiTotal;
};

#endif // APISTORAGE_H
