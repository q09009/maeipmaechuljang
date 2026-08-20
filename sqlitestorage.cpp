#include "sqlitestorage.h"

#include "transfersnapshot.h"

#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMetaType>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace {
QJsonValue nullableDateValue(const QVariant &value) {
    const QString date = value.toString().trimmed();
    return date.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(date.left(10));
}

QJsonValue numericValue(const QVariant &value) {
    return value.isNull() ? QJsonValue(0) : QJsonValue::fromVariant(value);
}

QString decimalValue(const QVariant &value) {
    return value.isNull() ? QStringLiteral("0") : value.toString();
}

QVariant nullableDateVariant(const QJsonValue &value) {
    if (value.isNull() || value.isUndefined() || value.toString().isEmpty())
        return QVariant(QMetaType::fromType<QString>());
    return value.toString();
}
} // namespace

SqliteStorage::SqliteStorage(const QString &connectionName, const QString &databasePath)
    : m_connectionName(connectionName),
      m_databasePath(databasePath.isEmpty()
                         ? QDir(QCoreApplication::applicationDirPath()).filePath("data/data.db")
                         : QFileInfo(databasePath).absoluteFilePath()) {}

SqliteStorage::~SqliteStorage() {
    if (m_db.isOpen()) {
        m_db.close();
    }
    const QString connectionName = m_db.connectionName();
    m_db = QSqlDatabase();
    if (!connectionName.isEmpty()) {
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool SqliteStorage::initDB() {
    QDir().mkpath(QFileInfo(m_databasePath).absolutePath());

    if (QSqlDatabase::contains(m_connectionName)) {
        m_db = QSqlDatabase::database(m_connectionName);
    } else {
        m_db = QSqlDatabase::addDatabase("QSQLITE", m_connectionName);
    }
    m_db.setDatabaseName(m_databasePath);

    if (m_db.open()) {
        qInfo() << "[DB_INIT] DB 연결 성공";
        QSqlQuery query(m_db);
        QString createTableQuery = R"(
            CREATE TABLE IF NOT EXISTS records (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                gubun TEXT, tr_date TEXT, customer TEXT, item TEXT, spec TEXT,
                price INTEGER, amount INTEGER, supply_val INTEGER, tax_val INTEGER, total_val INTEGER,
                pay_date1 TEXT, pay_amt1 INTEGER DEFAULT 0, pay_date2 TEXT, pay_amt2 INTEGER DEFAULT 0,
                pay_date3 TEXT, pay_amt3 INTEGER DEFAULT 0,
                unpaid_amt INTEGER GENERATED ALWAYS AS (
                    CASE WHEN gubun = '매입' THEN total_val - IFNULL(pay_amt1, 0) - IFNULL(pay_amt2, 0) - IFNULL(pay_amt3, 0) ELSE 0 END
                ) VIRTUAL,
                receivable_amt INTEGER GENERATED ALWAYS AS (
                    CASE WHEN gubun = '매출' THEN total_val - IFNULL(pay_amt1, 0) - IFNULL(pay_amt2, 0) - IFNULL(pay_amt3, 0) ELSE 0 END
                ) VIRTUAL
            );
        )";

        if (!query.exec(createTableQuery)) {
            qCritical() << "[DB_INIT] [FAIL] 레코드 테이블 생성 오류 -" << query.lastError().text();
        } else {
            qInfo() << "[DB_INIT] [SUCCESS] 레코드 테이블 준비 완료";
        }

        QString createCustomerTable = R"(
            CREATE TABLE IF NOT EXISTS customer (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE,
                balance INTEGER DEFAULT 0
            );
        )";
        if (!query.exec(createCustomerTable)) {
            qCritical() << "[DB_INIT] [FAIL] 거래처 테이블 생성 오류 -" << query.lastError().text();
        } else {
            qInfo() << "[DB_INIT] [SUCCESS] 거래처 테이블 준비 완료";
        }

        QString createItemTable = R"(
            CREATE TABLE IF NOT EXISTS item (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                item_name TEXT,
                spec TEXT,
                price REAL,
                UNIQUE(item_name, spec)
            );
        )";
        if (!query.exec(createItemTable)) {
            qCritical() << "[DB_INIT] [FAIL] 품목 테이블 생성 오류 -" << query.lastError().text();
        } else {
            qInfo() << "[DB_INIT] [SUCCESS] 품목 테이블 준비 완료";
        }
    } else {
        qCritical() << "[DB_INIT] [FAIL] DB 연결 실패 -" << m_db.lastError().text();
        return false;
    }

    initData();
    return true;
}

void SqliteStorage::syncExcelToSql(const QList<QStringList> &dataList) {
    qInfo() << "[SYNC_RECORD] [START] 전표 동기화 시작 - 총" << dataList.size() << "건";
    backupDB();
    if (!m_db.isOpen()) return;

    QSqlQuery deleteQuery(m_db);
    deleteQuery.exec("DELETE FROM records");
    deleteQuery.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'records'");

    m_db.transaction();

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO records (
            gubun, tr_date, customer, item, spec,
            price, amount, supply_val, tax_val, total_val,
            pay_date1, pay_amt1, pay_date2, pay_amt2,
            pay_date3, pay_amt3
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )");

    for (const QStringList &row : dataList) {
        if (row.size() < 16) continue;

        for (int i = 0; i < 16; ++i) {
            QString val = row.at(i);

            if ((i >= 5) && (val.isEmpty())) {
                query.addBindValue(QVariant(QMetaType(QMetaType::Double)));
            } else {
                query.addBindValue(val);
            }
        }
        query.exec();
    }

    if (m_db.commit()) {
        qInfo() << "[SYNC_RECORD] [COMPLETE] 전표 동기화 완료 - 총" << dataList.size() << "건";
    } else {
        qCritical() << "[SYNC_RECORD] [FAIL] 커밋 실패 -" << m_db.lastError().text();
        m_db.rollback();
    }
}

void SqliteStorage::syncExcelToSqlData(const QVariantList &customers, const QList<QStringList> &items) {
    qInfo() << "[SYNC_DATA] [START] 거래처/품목 동기화 시작 - 거래처" << customers.size() << "건 / 품목" << items.size() << "건";
    if (!m_db.isOpen()) return;

    QSqlQuery deleteQuery(m_db);
    deleteQuery.exec("DELETE FROM customer");
    deleteQuery.exec("DELETE FROM item");
    deleteQuery.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'customer'");
    deleteQuery.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'item'");

    m_db.transaction();

    QSqlQuery query(m_db);
    query.prepare(R"(
        INSERT INTO customer (name) VALUES (?);
    )");
    for (const QVariant &cust : customers) {
        query.addBindValue(cust);
        query.exec();
    }
    query.prepare(R"(
        INSERT INTO item (
            item_name, spec, price
        ) VALUES (?, ?, ?);
    )");

    for (const QStringList &item : items) {
        for (int i = 0; i < 3; i++) {
            query.addBindValue(item.at(i));
        }
        query.exec();
    }

    if (m_db.commit()) {
        qInfo() << "[SYNC_DATA] [COMPLETE] 거래처" << customers.size() << "건 / 품목" << items.size() << "건 동기화 완료";
    } else {
        qCritical() << "[SYNC_DATA] [FAIL] 커밋 실패 -" << m_db.lastError().text();
        m_db.rollback();
    }
}

void SqliteStorage::initData() {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.exec("SELECT * FROM customer ORDER BY name ASC");

    while (query.next()) {
        QVariant var;
        var = query.value(1).toString();
        dataName.append(var);
        int bal = query.value(2).toInt();
        dataBalance.append(bal);
    }

    query.exec("SELECT * FROM item ORDER BY item_name ASC");

    while (query.next()) {
        QVariantMap map;

        map["id"] = query.value(0);
        map["name"] = query.value(1);
        map["spec"] = query.value(2);
        map["price"] = query.value(3);
        dataProduct.append(map);
    }
}

void SqliteStorage::refreshData() {
    if (!m_db.isOpen()) return;

    dataName.clear();
    dataProduct.clear();
    dataBalance.clear();

    QSqlQuery query(m_db);
    query.exec("SELECT * FROM customer ORDER BY name ASC");

    while (query.next()) {
        QVariant var;
        var = query.value(1).toString();
        dataName.append(var);
        int bal = query.value(2).toInt();
        dataBalance.append(bal);
    }

    query.exec("SELECT * FROM item ORDER BY item_name ASC");

    while (query.next()) {
        QVariantMap map;

        map["id"] = query.value(0);
        map["name"] = query.value(1);
        map["spec"] = query.value(2);
        map["price"] = query.value(3);
        dataProduct.append(map);
    }
}

bool SqliteStorage::readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae,
                                    const QVariant &supplier, const QVariant &product) {
    if (!m_db.isOpen()) return false;

    searchedResult.clear();
    gaesoo = 0;

    QStringList a;
    a << "id" << "gb" << "date" << "supplier" << "product" << "size" << "price" << "quantity" << "gongga" << "buga" << "hapgye" << "ipD1" << "ipA1" << "ipD2" << "ipA2" << "ipD3" << "ipA3" << "miji" << "misu";

    QString gubun;
    if (mae) {
        gubun = "매입";
    } else {
        gubun = "매출";
    }

    qInfo() << "[READ_RANGE] [START] 조회 시작 -" << startDate.toString() << "~" << endDate.toString()
            << "/" << gubun << "/" << supplier.toString() << "/" << product.toString();
    QSqlQuery query(m_db);
    QString queryStr;
    QString sDate = startDate.toDate().toString("yyyy-MM-dd");
    QString eDate = endDate.toDate().toString("yyyy-MM-dd");
    queryStr = QString("SELECT * FROM records WHERE tr_date BETWEEN '%1' and '%2' AND gubun = '%3'")
                   .arg(sDate, eDate, gubun);
    if (supplier != "전체") {
        queryStr += QString(" AND customer = '%1'").arg(supplier.toString());
    }
    if (product != "전체") {
        queryStr += QString(" AND item = '%1'").arg(product.toString());
    }
    queryStr += QString(" ORDER BY tr_date ASC");
    query.exec(queryStr);

    while (query.next()) {
        QVariantMap map;
        gaesoo += 1;

        for (int i = 0; i < 19; i++) {
            map[a[i]] = query.value(i);
        }
        searchedResult.append(map);
    }

    calcSearchedSum(startDate, endDate, mae, supplier, product);
    qInfo() << "[READ_RANGE] [COMPLETE] 조회 완료 -" << gaesoo << "건 검색됨";
    return true;
}

void SqliteStorage::calcSearchedSum(const QVariant &startDate, const QVariant &endDate, bool mae,
                                    const QVariant &supplier, const QVariant &product) {
    amountSum = 0;
    gonggaSum = 0;
    bugaSum = 0;
    hapgyeSum = 0;
    ipamountSum = 0;
    misuSum = 0;
    mijiSum = 0;

    QString gubun;
    if (mae) {
        gubun = "매입";
    } else {
        gubun = "매출";
    }

    QSqlQuery query(m_db);
    QString queryStr;
    QString sDate = startDate.toDate().toString("yyyy-MM-dd");
    QString eDate = endDate.toDate().toString("yyyy-MM-dd");
    queryStr = QString("SELECT SUM(amount), SUM(supply_val), SUM(tax_val), SUM(total_val), SUM(pay_amt1 + pay_amt2 + pay_amt3), SUM(unpaid_amt), SUM(receivable_amt) FROM records WHERE tr_date BETWEEN '%1' and '%2' AND gubun = '%3'")
                   .arg(sDate, eDate, gubun);
    if (supplier != "전체") {
        queryStr += QString(" AND customer = '%1'").arg(supplier.toString());
    }
    if (product != "전체") {
        queryStr += QString(" AND item = '%1'").arg(product.toString());
    }

    if (query.exec(queryStr) && query.next()) {
        amountSum = query.value(0).toInt();
        gonggaSum = query.value(1).toInt();
        bugaSum = query.value(2).toInt();
        hapgyeSum = query.value(3).toInt();
        ipamountSum = query.value(4).toInt();
        mijiSum = query.value(5).toInt();
        misuSum = query.value(6).toInt();
    }
    ipamountSum = hapgyeSum.toInt() - mijiSum.toInt() - misuSum.toInt();
    qInfo() << "[CALC_SUM] [COMPLETE] 합계 계산 완료 - 수량합:" << amountSum.toInt()
            << "/ 공급가:" << gonggaSum.toInt() << "/ 부가세:" << bugaSum.toInt()
            << "/ 합계:" << hapgyeSum.toInt() << "/ 입금액:" << ipamountSum.toInt();
}

void SqliteStorage::monthTotalReady(const QVariant &year, const QVariant &gb, const QVariant &supplier,
                                    const QVariant &product) {
    if (!m_db.isOpen()) return;

    qInfo() << "[MONTH_STAT] [START] 월별통계 조회 -" << year.toString() << "년 /" << gb.toString()
            << "/" << supplier.toString() << "/" << product.toString();

    QString baseCondition = QString("WHERE strftime('%Y', tr_date) = '%1' AND gubun = '%2' ")
                                .arg(year.toString(), gb.toString());

    if (supplier.toString() != "전체")
        baseCondition += QString(" AND customer = '%1' ").arg(supplier.toString());
    if (product.toString() != "전체")
        baseCondition += QString(" AND item = '%1' ").arg(product.toString());

    calcMonthTotal(baseCondition);
    calcBungiTotal(baseCondition);
    calcBangiTotal(baseCondition);
}

void SqliteStorage::calcMonthTotal(QString queryStr) {
    monthTotal.clear();
    QSqlQuery query(m_db);
    QStringList a;
    a << "month" << "amount" << "gongga" << "buga" << "hapgye" << "miji" << "misu";

    QString sql = "SELECT strftime('%m', tr_date) as m, SUM(amount), SUM(supply_val), SUM(tax_val), SUM(total_val), SUM(unpaid_amt), SUM(receivable_amt)"
                  "FROM records " + queryStr + " GROUP BY m ORDER BY m;";

    if (query.exec(sql)) {
        while (query.next()) {
            QVariantMap map;
            for (int i = 0; i < 7; i++) {
                map[a[i]] = query.value(i);
            }
            monthTotal.append(map);
        }
    }
}

void SqliteStorage::calcBungiTotal(QString queryStr) {
    bungiTotal.clear();
    QSqlQuery query(m_db);

    QStringList a;
    a << "num" << "amount" << "gongga" << "buga" << "hapgye" << "miji" << "misu";

    QString sql = "SELECT (strftime('%m', tr_date)-1)/3 + 1 as q, SUM(amount), SUM(supply_val), SUM(tax_val), SUM(total_val), SUM(unpaid_amt), SUM(receivable_amt)"
                  "FROM records " + queryStr + " GROUP BY q ORDER BY q;";

    if (query.exec(sql)) {
        while (query.next()) {
            QVariantMap map;

            for (int i = 0; i < 7; i++) {
                map[a[i]] = query.value(i);
            }
            bungiTotal.append(map);
        }
    }
}

void SqliteStorage::calcBangiTotal(QString queryStr) {
    bangiTotal.clear();
    QSqlQuery query(m_db);
    QStringList a;
    a << "num" << "amount" << "gongga" << "buga" << "hapgye" << "miji" << "misu";

    QString sql = "SELECT "
                  "  CASE WHEN strftime('%m', tr_date) <= '06' THEN '1' ELSE '2' END AS h, "
                  "  SUM(amount), SUM(supply_val), SUM(tax_val), SUM(total_val), SUM(unpaid_amt), SUM(receivable_amt) "
                  "FROM records "
                  + queryStr
                  + " GROUP BY h ORDER BY h;";

    if (query.exec(sql)) {
        while (query.next()) {
            QVariantMap map;

            for (int i = 0; i < 7; i++) {
                map[a[i]] = query.value(i);
            }
            map["gb"] = "";
            bangiTotal.append(map);
        }
    }
}

void SqliteStorage::writeDataName(const QVariant &name) {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO customer (name) VALUES (:name)");
    query.bindValue(":name", name.toString());

    qInfo() << "[CUSTOMER_ADD] 거래처 추가 -" << name.toString();

    if (!query.exec()) {
        qCritical() << "[CUSTOMER_ADD] [FAIL] 거래처 추가 실패 -" << query.lastError().text();
    } else {
        qInfo() << "[CUSTOMER_ADD] [SUCCESS] 거래처 추가 완료 -" << name.toString();
    }
}

void SqliteStorage::writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price) {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO item (item_name, spec, price) VALUES (:product, :size, :price)");
    query.bindValue(":product", product.toString());
    query.bindValue(":size", size.toString());
    query.bindValue(":price", price.toString());

    qInfo() << "[ITEM_ADD] 품목 추가 -" << product.toString() << "/" << size.toString() << "/" << price.toString();

    if (!query.exec()) {
        qCritical() << "[ITEM_ADD] [FAIL] 품목 추가 실패 -" << query.lastError().text();
    } else {
        qInfo() << "[ITEM_ADD] [SUCCESS] 품목 추가 완료 -" << product.toString();
    }
}

void SqliteStorage::editDataSupplier(const QVariant &name, const QVariant &count) {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("UPDATE customer SET name = :name WHERE id = :id");
    query.bindValue(":name", name);
    query.bindValue(":id", count);

    qInfo() << "[CUSTOMER_EDIT] 거래처 수정 -" << name.toString();

    if (!query.exec()) {
        qCritical() << "[CUSTOMER_EDIT] [FAIL] 거래처 수정 실패 -" << query.lastError().text();
    } else {
        qInfo() << "[CUSTOMER_EDIT] [SUCCESS] 거래처 수정 완료 -" << name.toString();
    }
}

void SqliteStorage::editDataProduct(const QVariant &product, const QVariant &size, const QVariant &price,
                                    const QVariant &count) {
    if (!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("UPDATE item SET item_name = :name, spec = :size, price = :price WHERE id = :id");
    query.bindValue(":name", product);
    query.bindValue(":size", size.toString());
    query.bindValue(":price", price);
    query.bindValue(":id", count);

    qInfo() << "[ITEM_EDIT] 품목 수정 -" << product.toString() << "/" << size.toString() << "/" << price.toString();

    if (!query.exec()) {
        qCritical() << "[ITEM_EDIT] [FAIL] 품목 수정 실패 -" << query.lastError().text();
    } else {
        qInfo() << "[ITEM_EDIT] [SUCCESS] 품목 수정 완료 -" << product.toString();
    }
}

void SqliteStorage::writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2,
                                  const QVariant &amount2, const QVariant &date3, const QVariant &amount3,
                                  const QVariant &row) {
    if (!m_db.isOpen()) return;

    QDateTime dt1 = date1.toDateTime();
    QDate d1 = dt1.date();
    QDateTime dt2 = date2.toDateTime();
    QDate d2 = dt2.date();
    QDateTime dt3 = date3.toDateTime();
    QDate d3 = dt3.date();
    QSqlQuery query(m_db);
    query.prepare("UPDATE records SET pay_date1 = :d1, pay_amt1 = :a1, pay_date2 = :d2, pay_amt2 = :a2, pay_date3 = :d3, pay_amt3 = :a3 WHERE id = :id");
    query.bindValue(":d1", d1);
    query.bindValue(":d2", d2);
    query.bindValue(":d3", d3);
    query.bindValue(":a1", amount1);
    query.bindValue(":a2", amount2);
    query.bindValue(":a3", amount3);
    query.bindValue(":id", row);

    qInfo() << "[PAY_UPDATE] 입금내역 수정 - ID:" << row.toString()
            << "/ 입금1:" << d1 << amount1.toString() << "원"
            << "/ 입금2:" << d2 << amount2.toString() << "원"
            << "/ 입금3:" << d3 << amount3.toString() << "원";

    if (!query.exec()) {
        qCritical() << "[PAY_UPDATE] [FAIL] 입금내역 수정 실패 -" << query.lastError().text();
    } else {
        qInfo() << "[PAY_UPDATE] [SUCCESS] 입금내역 수정 완료 - ID:" << row.toString();
    }
}

void SqliteStorage::writeRecordIpFull(const QVariant &trDate, const QVariant &date1, const QVariant &amount1,
                                      const QVariant &date2, const QVariant &amount2, const QVariant &date3,
                                      const QVariant &amount3, const QVariant &row) {
    if (!m_db.isOpen()) return;

    QDate tr = trDate.toDate();
    QDateTime dt1 = date1.toDateTime();
    QDate d1 = dt1.date();
    QDateTime dt2 = date2.toDateTime();
    QDate d2 = dt2.date();
    QDateTime dt3 = date3.toDateTime();
    QDate d3 = dt3.date();
    QSqlQuery query(m_db);
    query.prepare("UPDATE records SET tr_date = :tr, pay_date1 = :d1, pay_amt1 = :a1, pay_date2 = :d2, pay_amt2 = :a2, pay_date3 = :d3, pay_amt3 = :a3 WHERE id = :id");
    query.bindValue(":tr", tr);
    query.bindValue(":d1", d1);
    query.bindValue(":a1", amount1);
    query.bindValue(":d2", d2);
    query.bindValue(":a2", amount2);
    query.bindValue(":d3", d3);
    query.bindValue(":a3", amount3);
    query.bindValue(":id", row);

    qInfo() << "[PAY_UPDATE_FULL] 입금내역+거래날짜 수정 - ID:" << row.toString()
            << "/ 거래날짜:" << tr
            << "/ 입금1:" << d1 << amount1.toString() << "원"
            << "/ 입금2:" << d2 << amount2.toString() << "원"
            << "/ 입금3:" << d3 << amount3.toString() << "원";

    if (!query.exec()) {
        qCritical() << "[PAY_UPDATE_FULL] [FAIL] 입금내역 수정 실패 -" << query.lastError().text();
    } else {
        qInfo() << "[PAY_UPDATE_FULL] [SUCCESS] 입금내역+거래날짜 수정 완료 - ID:" << row.toString();
    }
}

void SqliteStorage::deleteDataSupplier(const QVariant &id) {
    if (!m_db.isOpen()) return;

    QString customerName = "알 수 없음";

    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT name FROM customer WHERE id = :id");
    checkQuery.bindValue(":id", id);
    if (checkQuery.exec() && checkQuery.next()) {
        customerName = checkQuery.value(0).toString();
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM customer WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec()) {
        qInfo() << "[CUSTOMER_DEL] [SUCCESS] 거래처 삭제 완료 - (ID:" << id.toString() << ")" << customerName;
    } else {
        qCritical() << "[CUSTOMER_DEL] [FAIL] 거래처 삭제 실패 - (ID:" << id.toString() << ")" << customerName
                    << "사유:" << query.lastError().text();
    }
}

void SqliteStorage::deleteDataProduct(const QVariant &id) {
    if (!m_db.isOpen()) return;

    QString itemName = "알 수 없음";

    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT item_name FROM item WHERE id = :id");
    checkQuery.bindValue(":id", id);
    if (checkQuery.exec() && checkQuery.next()) {
        itemName = checkQuery.value(0).toString();
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM item WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec()) {
        qInfo() << "[ITEM_DEL] [SUCCESS] 품목 삭제 완료 - (ID:" << id.toString() << ")" << itemName;
    } else {
        qCritical() << "[ITEM_DEL] [FAIL] 품목 삭제 실패 - (ID:" << id.toString() << ")" << itemName
                    << "사유:" << query.lastError().text();
    }
}

void SqliteStorage::deleteRecord(const QVariant &id) {
    if (!m_db.isOpen()) return;

    QString recordInfo = "정보 없음";

    QSqlQuery checkQuery(m_db);
    checkQuery.prepare("SELECT tr_date, customer, item, total_val FROM records WHERE id = :id");
    checkQuery.bindValue(":id", id);
    if (checkQuery.exec() && checkQuery.next()) {
        recordInfo = QString("[%1 | %2 | %3 | %4원]")
                         .arg(checkQuery.value(0).toString())
                         .arg(checkQuery.value(1).toString())
                         .arg(checkQuery.value(2).toString())
                         .arg(checkQuery.value(3).toString());
    }

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM records WHERE id = :id");
    query.bindValue(":id", id);

    if (query.exec()) {
        qInfo() << "[RECORD_DEL] [SUCCESS] 전표 기록 삭제 완료 - (ID:" << id.toString() << ")" << recordInfo;
    } else {
        qCritical() << "[RECORD_DEL] [FAIL] 전표 기록 삭제 실패 - (ID:" << id.toString() << ")" << recordInfo
                    << "사유:" << query.lastError().text();
    }
}

void SqliteStorage::writeExcelRecord(const bool &mae, const QVariant &date, const QVariant &supplier,
                                     const QVariant &product, const QVariant &size, const QVariant &price,
                                     const QVariant &quantity, const bool &tax) {
    if (!m_db.isOpen()) return;

    QString gb;
    int gongga = price.toInt() * quantity.toInt();
    int buga = 0;
    if (tax) {
        buga = gongga / 10;
    }
    if (mae) {
        gb = QString("매입");
    } else {
        gb = QString("매출");
    }

    QDateTime dateTime = date.toDateTime();
    QDate dateOnly = dateTime.date();

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO records (gubun, tr_date, customer, item, spec, price, amount, supply_val, tax_val, total_val) VALUES (:gb, :date, :sup, :pro, :size, :price, :amt, :gongga, :buga, :hap)");
    query.bindValue(":gb", gb);
    query.bindValue(":date", dateOnly);
    query.bindValue(":sup", supplier.toString());
    query.bindValue(":pro", product.toString());
    query.bindValue(":size", size.toString());
    query.bindValue(":price", price);
    query.bindValue(":amt", quantity);
    query.bindValue(":gongga", gongga);
    query.bindValue(":buga", buga);
    query.bindValue(":hap", gongga + buga);

    qInfo() << "[RECORD_ADD] 전표 추가 -" << gb << "/" << dateOnly.toString()
            << "/" << supplier.toString() << "/" << product.toString()
            << "/" << size.toString() << "/ 단가:" << price.toInt()
            << "/ 수량:" << quantity.toInt() << "/ 공급가:" << gongga << "/ 부가세:" << buga << "/ 합계:" << gongga + buga;

    if (!query.exec()) {
        qCritical() << "[RECORD_ADD] [FAIL] 전표 추가 실패 -" << query.lastError().text();
    } else {
        qInfo() << "[RECORD_ADD] [SUCCESS] 전표 추가 완료 -" << gb << dateOnly.toString() << supplier.toString();
    }
}

QList<QStringList> SqliteStorage::readAllSqlRecord() {
    QList<QStringList> list;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM records");

    if (query.exec()) {
        while (query.next()) {
            QStringList data;
            for (int i = 1; i < 19; i++) {
                data << query.value(i).toString();
            }
            list.append(data);
        }
    }
    qInfo() << "[READ_ALL] 전체 전표 조회 완료 -" << list.size() << "건";
    return list;
}

QList<QStringList> SqliteStorage::readAllSqlItem() {
    QList<QStringList> list;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM item");

    if (query.exec()) {
        while (query.next()) {
            QStringList data;
            for (int i = 1; i < 4; i++) {
                data << query.value(i).toString();
            }
            list.append(data);
        }
    }
    qInfo() << "[READ_ALL] 전체 품목 조회 완료 -" << list.size() << "건";
    return list;
}

QVariantList SqliteStorage::readAllSqlCustomer() {
    QVariantList list;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM customer");

    if (query.exec()) {
        while (query.next()) {
            QVariant data;
            data = query.value(1);
            list.append(data);
        }
    }
    qInfo() << "[READ_ALL] 전체 거래처 조회 완료 -" << list.size() << "건";
    return list;
}

void SqliteStorage::writeRecordIlgwalIpgeum(const QVariant &date, const QVariant &amount) {
    if (searchedResult.isEmpty()) return;

    QList<int> idList;
    QList<int> ipWhere;
    QList<int> ipAmt;

    QVariantMap test = searchedResult.at(0).toMap();
    QString customerName = test["supplier"].toString();
    bool isMae = (test["gb"].toString() == "매입");
    QDate insertDate = date.toDate();

    int remainingAmt = amount.toInt();
    for (int i = 0; i < dataName.size(); i++) {
        if (dataName[i] == customerName) {
            remainingAmt += dataBalance[i];
            break;
        }
    }
    int customerId = -1;
    QSqlQuery idQuery(m_db);
    idQuery.prepare("SELECT id FROM customer WHERE name = :name");
    idQuery.bindValue(":name", customerName);
    if (idQuery.exec() && idQuery.next()) {
        customerId = idQuery.value(0).toInt();
    }

    qInfo() << "[BULK_PAY] [START] 일괄입금 시작 -" << customerName << "/ 처리금액:" << remainingAmt << "원";

    QSqlQuery query(m_db);
    m_db.transaction();

    QString resetQuery = QString("UPDATE customer SET balance = 0 WHERE name = '%1'").arg(customerName);
    query.exec(resetQuery);

    for (const QVariant &item : searchedResult) {
        QVariantMap map = item.toMap();
        int targetAmt = (isMae ? map["miji"].toInt() : map["misu"].toInt());

        if (targetAmt > 0) {
            idList.append(map["id"].toInt());

            if (map["ipA1"].toInt() == 0) {
                ipWhere.append(1);
            } else if (map["ipA2"].toInt() == 0) {
                ipWhere.append(2);
            } else {
                ipWhere.append(3);
            }
            ipAmt.append(targetAmt);
        }
    }

    for (int i = 0; i < idList.size(); i++) {
        if (remainingAmt <= 0) break;

        int howMuch = ipAmt[i];
        int ip = (remainingAmt >= howMuch) ? howMuch : remainingAmt;
        remainingAmt -= ip;

        QString dateStr = insertDate.toString("yyyy-MM-dd");
        QString queryStr = "UPDATE records SET ";

        if (ipWhere[i] == 1) {
            queryStr += QString("pay_date1 = '%1', pay_amt1 = %2").arg(dateStr).arg(ip);
        } else if (ipWhere[i] == 2) {
            queryStr += QString("pay_date2 = '%1', pay_amt2 = %2").arg(dateStr).arg(ip);
        } else {
            queryStr += QString("pay_date3 = '%1', pay_amt3 = IFNULL(pay_amt3, 0) + %2").arg(dateStr).arg(ip);
        }

        queryStr += QString(" WHERE id = %1").arg(idList[i]);
        query.exec(queryStr);
        qInfo() << "[BULK_PAY] 입금처리중 -" << ip << "원 입금 / 잔액:" << remainingAmt << "원";
    }

    QSqlQuery finalQuery(m_db);
    QString porm = (isMae ? "+" : "-");
    QString customerQuery = QString("UPDATE customer SET balance = balance %3 %1 WHERE id = %2")
                                .arg(remainingAmt)
                                .arg(customerId)
                                .arg(porm);

    if (finalQuery.exec(customerQuery)) {
        int affected = finalQuery.numRowsAffected();
        qDebug() << "쿼리 성공! 바뀐 줄 수:" << affected;

        if (affected == 0) {
            qDebug() << "주의: 업데이트는 성공했지만 실제 바뀐 데이터가 없음(ID가 틀렸을 수도!)";
        }
    } else {
        qDebug() << "쿼리 실패 에러:" << finalQuery.lastError().text();
    }

    if (!m_db.commit()) {
        qDebug() << "커밋 실패:" << m_db.lastError().text();
    } else {
        qDebug() << "커밋 완료!";
    }
    refreshData();
}

QList<QStringList> SqliteStorage::readMonthlySql(const QVariant &year, const QVariant &month) {
    QList<QStringList> list;

    QSqlQuery query(m_db);
    query.prepare("SELECT gubun, tr_date, customer, item, spec, price, amount, supply_val, tax_val, total_val "
                  "FROM records "
                  "WHERE strftime('%Y', tr_date) = :y "
                  "AND strftime('%m', tr_date) = :m");

    query.bindValue(":y", QString::number(year.toInt()));
    query.bindValue(":m", QString("%1").arg(month.toInt(), 2, 10, QChar('0')));

    if (query.exec()) {
        while (query.next()) {
            QStringList data;
            for (int i = 0; i < 10; i++) {
                data << query.value(i).toString();
            }
            list.append(data);
        }
    }
    qInfo() << "[READ_MONTHLY] 월별 전표 조회 완료 -" << year.toInt() << "년" << month.toInt() << "월 /" << list.size() << "건";
    return list;
}

QJsonObject SqliteStorage::exportTransferSnapshot(QString *error) {
    if (error)
        error->clear();
    if (!m_db.isOpen()) {
        if (error)
            *error = "SQLite 데이터베이스가 열려 있지 않습니다";
        return {};
    }

    QJsonArray customers;
    QSqlQuery customerQuery(m_db);
    if (!customerQuery.exec("SELECT name, balance FROM customer ORDER BY name, id")) {
        if (error)
            *error = customerQuery.lastError().text();
        return {};
    }
    while (customerQuery.next()) {
        customers.append(QJsonObject{
            {"name", customerQuery.value(0).toString()},
            {"balance", numericValue(customerQuery.value(1))},
        });
    }

    QJsonArray items;
    QSqlQuery itemQuery(m_db);
    if (!itemQuery.exec("SELECT item_name, spec, price FROM item ORDER BY item_name, spec, id")) {
        if (error)
            *error = itemQuery.lastError().text();
        return {};
    }
    while (itemQuery.next()) {
        items.append(QJsonObject{
            {"item_name", itemQuery.value(0).toString()},
            {"spec", itemQuery.value(1).toString()},
            {"price", decimalValue(itemQuery.value(2))},
        });
    }

    QJsonArray records;
    QSqlQuery recordQuery(m_db);
    if (!recordQuery.exec(
            "SELECT gubun, tr_date, customer, item, spec, price, amount, supply_val, "
            "tax_val, total_val, pay_date1, pay_amt1, pay_date2, pay_amt2, pay_date3, "
            "pay_amt3, id FROM records ORDER BY tr_date, id")) {
        if (error)
            *error = recordQuery.lastError().text();
        return {};
    }
    while (recordQuery.next()) {
        records.append(QJsonObject{
            {"source_id", numericValue(recordQuery.value(16))},
            {"gubun", recordQuery.value(0).toString()},
            {"tr_date", recordQuery.value(1).toString().left(10)},
            {"customer", recordQuery.value(2).toString()},
            {"item", recordQuery.value(3).toString()},
            {"spec", recordQuery.value(4).toString()},
            {"price", numericValue(recordQuery.value(5))},
            {"amount", numericValue(recordQuery.value(6))},
            {"supply_val", numericValue(recordQuery.value(7))},
            {"tax_val", numericValue(recordQuery.value(8))},
            {"total_val", numericValue(recordQuery.value(9))},
            {"pay_date1", nullableDateValue(recordQuery.value(10))},
            {"pay_amt1", numericValue(recordQuery.value(11))},
            {"pay_date2", nullableDateValue(recordQuery.value(12))},
            {"pay_amt2", numericValue(recordQuery.value(13))},
            {"pay_date3", nullableDateValue(recordQuery.value(14))},
            {"pay_amt3", numericValue(recordQuery.value(15))},
        });
    }
    return QJsonObject{
        {"version", 1},
        {"customers", customers},
        {"items", items},
        {"records", records},
    };
}

bool SqliteStorage::createBackupWithPrefix(const QString &prefix, QString *backupReference,
                                           QString *error) {
    if (backupReference)
        backupReference->clear();
    if (error)
        error->clear();
    if (!m_db.isOpen()) {
        if (error)
            *error = "SQLite 데이터베이스가 열려 있지 않습니다";
        return false;
    }

    const QString backupDir = QFileInfo(m_databasePath).dir().filePath("backups");
    if (!QDir().mkpath(backupDir)) {
        if (error)
            *error = "SQLite 백업 디렉터리를 만들 수 없습니다: " + backupDir;
        return false;
    }
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");
    const QString suffix = QUuid::createUuid().toString(QUuid::Id128).left(8);
    const QString destination = QDir(backupDir).filePath(
        QString("%1_%2_%3.db").arg(prefix, timestamp, suffix));

    QSqlQuery checkpoint(m_db);
    checkpoint.exec("PRAGMA wal_checkpoint(FULL)");
    checkpoint.finish();
    QString escaped = destination;
    escaped.replace(QChar(0x27), QStringLiteral("''"));
    QSqlQuery backupQuery(m_db);
    if (!backupQuery.exec(QString("VACUUM INTO '%1'").arg(escaped))) {
        if (error)
            *error = backupQuery.lastError().text();
        qCritical() << "[BACKUP] SQLite 백업 실패:" << backupQuery.lastError().text();
        return false;
    }
    if (backupReference)
        *backupReference = destination;
    qInfo() << "[BACKUP] SQLite 백업 성공:" << destination;
    return true;
}

bool SqliteStorage::createTransferBackup(QString *backupReference, QString *error) {
    return createBackupWithPrefix("transfer_backup", backupReference, error);
}

bool SqliteStorage::replaceTransferSnapshot(const QJsonObject &snapshot,
                                            QString *backupReference, QString *error) {
    if (error)
        error->clear();
    QString validationError;
    if (!TransferSnapshot::validate(snapshot, &validationError)) {
        if (error)
            *error = validationError;
        return false;
    }
    if (!createTransferBackup(backupReference, error))
        return false;
    if (!m_db.transaction()) {
        if (error)
            *error = m_db.lastError().text();
        return false;
    }

    auto rollbackWith = [&](const QString &message) {
        m_db.rollback();
        if (error)
            *error = message;
        return false;
    };

    QSqlQuery clearQuery(m_db);
    for (const QString &statement : {
             "DELETE FROM records", "DELETE FROM customer", "DELETE FROM item",
             "DELETE FROM sqlite_sequence WHERE name IN ('records', 'customer', 'item')"}) {
        if (!clearQuery.exec(statement))
            return rollbackWith(clearQuery.lastError().text());
    }

    QSqlQuery customerQuery(m_db);
    customerQuery.prepare("INSERT INTO customer (name, balance) VALUES (?, ?)");
    for (const QJsonValue &value : snapshot.value("customers").toArray()) {
        const QJsonObject row = value.toObject();
        customerQuery.bindValue(0, row.value("name").toString());
        customerQuery.bindValue(1, row.value("balance").toVariant().toLongLong());
        if (!customerQuery.exec())
            return rollbackWith(customerQuery.lastError().text());
    }

    QSqlQuery itemQuery(m_db);
    itemQuery.prepare("INSERT INTO item (item_name, spec, price) VALUES (?, ?, ?)");
    for (const QJsonValue &value : snapshot.value("items").toArray()) {
        const QJsonObject row = value.toObject();
        itemQuery.bindValue(0, row.value("item_name").toString());
        itemQuery.bindValue(1, row.value("spec").toString());
        itemQuery.bindValue(2, row.value("price").toVariant());
        if (!itemQuery.exec())
            return rollbackWith(itemQuery.lastError().text());
    }

    QSqlQuery recordInsert(m_db);
    recordInsert.prepare(
        "INSERT INTO records (gubun, tr_date, customer, item, spec, price, amount, "
        "supply_val, tax_val, total_val, pay_date1, pay_amt1, pay_date2, pay_amt2, "
        "pay_date3, pay_amt3) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    for (const QJsonValue &value : snapshot.value("records").toArray()) {
        const QJsonObject row = value.toObject();
        recordInsert.bindValue(0, row.value("gubun").toString());
        recordInsert.bindValue(1, row.value("tr_date").toString());
        recordInsert.bindValue(2, row.value("customer").toString());
        recordInsert.bindValue(3, row.value("item").toString());
        recordInsert.bindValue(4, row.value("spec").toString());
        recordInsert.bindValue(5, row.value("price").toVariant().toLongLong());
        recordInsert.bindValue(6, row.value("amount").toVariant().toLongLong());
        recordInsert.bindValue(7, row.value("supply_val").toVariant().toLongLong());
        recordInsert.bindValue(8, row.value("tax_val").toVariant().toLongLong());
        recordInsert.bindValue(9, row.value("total_val").toVariant().toLongLong());
        recordInsert.bindValue(10, nullableDateVariant(row.value("pay_date1")));
        recordInsert.bindValue(11, row.value("pay_amt1").toVariant().toLongLong());
        recordInsert.bindValue(12, nullableDateVariant(row.value("pay_date2")));
        recordInsert.bindValue(13, row.value("pay_amt2").toVariant().toLongLong());
        recordInsert.bindValue(14, nullableDateVariant(row.value("pay_date3")));
        recordInsert.bindValue(15, row.value("pay_amt3").toVariant().toLongLong());
        if (!recordInsert.exec())
            return rollbackWith(recordInsert.lastError().text());
    }

    QString verifyError;
    const QJsonObject actual = exportTransferSnapshot(&verifyError);
    if (!verifyError.isEmpty())
        return rollbackWith("SQLite 검증 데이터 읽기 실패: " + verifyError);
    if (TransferSnapshot::digest(actual) != TransferSnapshot::digest(snapshot))
        return rollbackWith("SQLite에 기록된 데이터가 원본 스냅샷과 일치하지 않습니다");
    if (!m_db.commit())
        return rollbackWith(m_db.lastError().text());

    refreshData();
    return true;
}

bool SqliteStorage::backupDB() {
    QString backupReference;
    QString error;
    return createBackupWithPrefix("backup", &backupReference, &error);
}

void SqliteStorage::cleanOldBackups() {
    QString backupDir = QCoreApplication::applicationDirPath() + "/data/backups";
    QDir dir(backupDir);

    if (!dir.exists()) return;

    QStringList filters;
    filters << "backup_*.db";
    dir.setNameFilters(filters);

    dir.setSorting(QDir::Time | QDir::Reversed);
    QFileInfoList list = dir.entryInfoList();

    QDateTime now = QDateTime::currentDateTime();
    int daysToKeep = 7;

    qInfo() << "[CLEANUP] 오래된 백업 확인 중...";

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);

        qint64 daysDiff = fileInfo.lastModified().daysTo(now);

        if (daysDiff > daysToKeep) {
            if (QFile::remove(fileInfo.absoluteFilePath())) {
                qInfo() << "[CLEANUP] 삭제됨 (오래된 파일):" << fileInfo.fileName();
            } else {
                qWarning() << "[CLEANUP] 삭제 실패:" << fileInfo.fileName();
            }
        }
    }
}

void SqliteStorage::cleanOldLogs() {
    QString logDir = QCoreApplication::applicationDirPath() + "/logs";
    QDir dir(logDir);

    if (!dir.exists()) return;

    QStringList filters;
    filters << "log_*.txt";
    dir.setNameFilters(filters);

    dir.setSorting(QDir::Name);
    QFileInfoList list = dir.entryInfoList();

    QDateTime now = QDateTime::currentDateTime();
    int daysToKeep = 7;

    qInfo() << "[CLEANUP] 오래된 로그 확인 중...";

    for (int i = 0; i < list.size(); ++i) {
        QFileInfo fileInfo = list.at(i);

        QString dateStr = fileInfo.baseName().mid(4);
        QDate fileDate = QDate::fromString(dateStr, "yyyy-MM-dd");
        qint64 actualDaysDiff = fileDate.daysTo(now.date());

        if (actualDaysDiff > daysToKeep) {
            if (QFile::remove(fileInfo.absoluteFilePath())) {
                qInfo() << "[CLEANUP] 삭제됨 (오래된 파일):" << fileInfo.fileName();
            } else {
                qWarning() << "[CLEANUP] 삭제 실패:" << fileInfo.fileName();
            }
        }
    }
}

QVariantList SqliteStorage::getDataName() const {
    return dataName;
}

QVariantList SqliteStorage::getDataProduct() const {
    return dataProduct;
}

QVariantList SqliteStorage::getSearchedResult() const {
    return searchedResult;
}

QVariant SqliteStorage::getAmountSum() const {
    return amountSum;
}

QVariant SqliteStorage::getGonggaSum() const {
    return gonggaSum;
}

QVariant SqliteStorage::getBugaSum() const {
    return bugaSum;
}

QVariant SqliteStorage::getHapgyeSum() const {
    return hapgyeSum;
}

QVariant SqliteStorage::getIpamountSum() const {
    return ipamountSum;
}

QVariant SqliteStorage::getMisuSum() const {
    return misuSum;
}

QVariant SqliteStorage::getMijiSum() const {
    return mijiSum;
}

QVariantList SqliteStorage::getMonthTotal() const {
    return monthTotal;
}

QVariantList SqliteStorage::getBungiTotal() const {
    return bungiTotal;
}

QVariantList SqliteStorage::getBangiTotal() const {
    return bangiTotal;
}

int SqliteStorage::getGaesoo() const {
    return gaesoo;
}
