#include "sqlitestorage.h"
#include "transfersnapshot.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class DataTransferTests : public QObject {
    Q_OBJECT

private:
    static QJsonObject sampleSnapshot() {
        return {
            {"version", 1},
            {"customers", QJsonArray{
                QJsonObject{{"name", "거래처"}, {"balance", 1200}},
            }},
            {"items", QJsonArray{
                QJsonObject{{"item_name", "품목"}, {"spec", "EA"}, {"price", "1000.00"}},
            }},
            {"records", QJsonArray{
                QJsonObject{
                    {"gubun", "매출"}, {"tr_date", "2026-08-21"},
                    {"customer", "거래처"}, {"item", "품목"}, {"spec", "EA"},
                    {"price", 1000}, {"amount", 2}, {"supply_val", 2000},
                    {"tax_val", 200}, {"total_val", 2200},
                    {"pay_date1", "2026-08-22"}, {"pay_amt1", 1000},
                    {"pay_date2", QJsonValue(QJsonValue::Null)}, {"pay_amt2", 0},
                    {"pay_date3", QJsonValue(QJsonValue::Null)}, {"pay_amt3", 0},
                },
            }},
        };
    }

private slots:
    void copiesAndVerifiesSnapshot() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString sourcePath = temporary.filePath("source/data.db");
        const QString targetPath = temporary.filePath("target/data.db");

        SqliteStorage source("transfer_test_source", sourcePath);
        QVERIFY(source.initDB());
        QString sourceBackup;
        QString error;
        QVERIFY2(source.replaceTransferSnapshot(sampleSnapshot(), &sourceBackup, &error),
                 qPrintable(error));
        QVERIFY(QFileInfo::exists(sourceBackup));

        const QJsonObject exported = source.exportTransferSnapshot(&error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(TransferSnapshot::counts(exported).value("customers").toInt(), 1);
        QCOMPARE(TransferSnapshot::counts(exported).value("items").toInt(), 1);
        QCOMPARE(TransferSnapshot::counts(exported).value("records").toInt(), 1);

        SqliteStorage target("transfer_test_target", targetPath);
        QVERIFY(target.initDB());
        QString targetBackup;
        QVERIFY2(target.replaceTransferSnapshot(exported, &targetBackup, &error),
                 qPrintable(error));
        QVERIFY(QFileInfo::exists(targetBackup));
        const QJsonObject copied = target.exportTransferSnapshot(&error);
        QCOMPARE(TransferSnapshot::digest(copied), TransferSnapshot::digest(exported));
    }

    void rollsBackInvalidReplacement() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        SqliteStorage storage("transfer_test_rollback", temporary.filePath("data/data.db"));
        QVERIFY(storage.initDB());
        QString backup;
        QString error;
        QVERIFY(storage.replaceTransferSnapshot(sampleSnapshot(), &backup, &error));
        const QJsonObject before = storage.exportTransferSnapshot(&error);

        QJsonObject duplicate = sampleSnapshot();
        QJsonArray customers = duplicate.value("customers").toArray();
        customers.append(customers.first());
        duplicate["customers"] = customers;
        QVERIFY(!storage.replaceTransferSnapshot(duplicate, &backup, &error));
        QVERIFY(!error.isEmpty());
        QVERIFY(QFileInfo::exists(backup));

        error.clear();
        const QJsonObject after = storage.exportTransferSnapshot(&error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(TransferSnapshot::digest(after), TransferSnapshot::digest(before));
    }

    void normalizesLegacyNullValues() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        const QString connectionName = "transfer_test_legacy_nulls";
        SqliteStorage storage(connectionName, temporary.filePath("data/data.db"));
        QVERIFY(storage.initDB());
        {
            QSqlDatabase database = QSqlDatabase::database(connectionName);
            QSqlQuery query(database);
            QVERIFY(query.exec("INSERT INTO customer (name, balance) VALUES ('거래처', 0)"));
            QVERIFY(query.exec("INSERT INTO item (item_name, spec, price) VALUES ('품목', NULL, NULL)"));
            QVERIFY(query.exec(
                "INSERT INTO records (gubun, tr_date, customer, item, spec, price, amount, "
                "supply_val, tax_val, total_val, pay_amt1, pay_amt2, pay_amt3) VALUES "
                "('매출', '2026-08-21', '거래처', '품목', NULL, NULL, NULL, 100, NULL, 100, "
                "NULL, NULL, NULL)"));
        }

        QString error;
        const QJsonObject snapshot = storage.exportTransferSnapshot(&error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QVERIFY2(TransferSnapshot::validate(snapshot, &error), qPrintable(error));
        const QJsonObject item = snapshot.value("items").toArray().first().toObject();
        QCOMPARE(item.value("spec").toString(), QString());
        QCOMPARE(item.value("price").toString(), QString("0"));
        const QJsonObject record = snapshot.value("records").toArray().first().toObject();
        QCOMPARE(record.value("spec").toString(), QString());
        QCOMPARE(record.value("price").toInt(), 0);
        QCOMPARE(record.value("amount").toInt(), 0);
        QCOMPARE(record.value("tax_val").toInt(), 0);
        QCOMPARE(record.value("pay_amt1").toInt(), 0);
    }

    void backsUpAndVerifiesEmptyReset() {
        QTemporaryDir temporary;
        QVERIFY(temporary.isValid());
        SqliteStorage storage("transfer_test_reset", temporary.filePath("data/data.db"));
        QVERIFY(storage.initDB());

        QString backup;
        QString error;
        QVERIFY2(storage.replaceTransferSnapshot(sampleSnapshot(), &backup, &error),
                 qPrintable(error));

        const QJsonObject emptySnapshot{
            {"version", 1},
            {"customers", QJsonArray{}},
            {"items", QJsonArray{}},
            {"records", QJsonArray{}},
        };
        backup.clear();
        error.clear();
        QVERIFY2(storage.replaceTransferSnapshot(emptySnapshot, &backup, &error),
                 qPrintable(error));
        QVERIFY(QFileInfo::exists(backup));

        const QJsonObject actual = storage.exportTransferSnapshot(&error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(TransferSnapshot::counts(actual).value("customers").toInt(), 0);
        QCOMPARE(TransferSnapshot::counts(actual).value("items").toInt(), 0);
        QCOMPARE(TransferSnapshot::counts(actual).value("records").toInt(), 0);
        QCOMPARE(TransferSnapshot::digest(actual), TransferSnapshot::digest(emptySnapshot));
    }
};

QTEST_GUILESS_MAIN(DataTransferTests)
#include "data_transfer_tests.moc"
