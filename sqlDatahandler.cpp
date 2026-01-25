#include "sqlDatahandler.h"
#include "excelDatahandler.h"
#include <QSqlError>
#include <QDebug>
#include <QSqlDatabase>
#include <QDir>
#include <QStandardPaths>

SqlHandler::SqlHandler(QObject *parent) : QObject(parent) {}

SqlHandler::~SqlHandler() {
    if (m_db.isOpen()) m_db.close();
}

void SqlHandler::initDB() {
    QString path = "data";
    QDir dir(path);

    if (!dir.exists()) {
        dir.mkpath(".");
    }

    // 1. 멤버 변수에 직접 할당하고, "연결 이름"을 다르게 줘야 합니다!
    // SQLite는 기본적으로 하나의 연결을 쓰지만, 여러 개를 쓸 땐 이름을 지어줘야 안 꼬입니다.

    m_db = QSqlDatabase::addDatabase("QSQLITE", "data_connection");
    m_db.setDatabaseName(path + "/data.db");

    // 2. record DB 처리
    if (m_db.open()) {
        // 쿼리를 만들 때 어떤 DB를 쓸지 꼭 알려줘야 합니다 (m_recorddb)
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
            qDebug() << "레코드 테이블 생성 에러:" << query.lastError().text();
        }

        // 1. 거래처 테이블 생성
        QString createCustomerTable = R"(
            CREATE TABLE IF NOT EXISTS customer (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE  -- 중복 방지를 위해 UNIQUE 추가하면 좋습니다
            );
        )";
        if (!query.exec(createCustomerTable)) {
            qDebug() << "거래처 테이블 생성 에러:" << query.lastError().text();
        }

        // 2. 품목 테이블 생성 (규격, 단가 포함)
        QString createItemTable = R"(
            CREATE TABLE IF NOT EXISTS item (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                item_name TEXT,
                spec TEXT,
                price REAL,
                UNIQUE(item_name, spec) -- 품목명과 규격이 동시에 같은 데이터는 안 들어가게 설정
            );
        )";
        if (!query.exec(createItemTable)) {
            qDebug() << "품목 테이블 생성 에러:" << query.lastError().text();
        }
    }
    else {
        qDebug() << "db 오픈 실패: " << m_db.lastError().text();
    }

    initData();
}

void SqlHandler::syncExcelToSql(const QList<QStringList>& dataList) {
    if (!m_db.isOpen()) return;

    // 1. 기존 데이터 삭제 (새로 동기화할 때 중복 방지)
    QSqlQuery deleteQuery(m_db);
    deleteQuery.exec("DELETE FROM records");
    // id 카운터 초기화
    deleteQuery.exec("UPDATE sqlite_sequence SET seq = 0 WHERE name = 'records'");

    // 2. 트랜잭션 시작 (속도의 비결!)
    m_db.transaction();

    QSqlQuery query(m_db);
    // 18개 항목에 대응하는 INSERT 문 준비
    query.prepare(R"(
        INSERT INTO records (
            gubun, tr_date, customer, item, spec,
            price, amount, supply_val, tax_val, total_val,
            pay_date1, pay_amt1, pay_date2, pay_amt2,
            pay_date3, pay_amt3
        ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
    )");

    // 3. 데이터 바인딩 루프
    for (const QStringList& row : dataList) {
        if (row.size() < 16) continue;

        for (int i = 0; i < 16; ++i) {
            QString val = row.at(i);

            // 숫자가 들어가야 할 칸(6번 인덱스부터 끝까지)이 "0"이거나 비었으면 NULL 처리
            if ((i >= 5) && (val.isEmpty() || val == "0")) {
                query.addBindValue(QVariant(QMetaType(QMetaType::Double)));
            } else {
                query.addBindValue(val);
            }
        }
        query.exec();
    }

    // 4. 한 번에 저장 확정
    if (m_db.commit()) {
        qDebug() << "총" << dataList.size() << "건의 데이터 동기화 완료!";
    } else {
        qDebug() << "커밋 실패:" << m_db.lastError().text();
        m_db.rollback(); // 실패 시 되돌리기
    }
}

void SqlHandler::syncExcelToSqlData(const QVariantList &customers, const QList<QStringList>& items) {
    if(!m_db.isOpen()) return;

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
    for(const QVariant& cust : customers) {
        query.addBindValue(cust);
        query.exec();
    }
    query.prepare(R"(
        INSERT INTO item (
            item_name, spec, price
        ) VALUES (?, ?, ?);
    )");

    for(const QStringList& item : items) {
        for(int i=0;i<3;i++) {
            query.addBindValue(item.at(i));
        }
        query.exec();
    }

    if (m_db.commit()) {
        qDebug() << "총" << customers.size() << "건의 거래처명 동기화 완료!";
        qDebug() << "총" << items.size() << "건의 상품들 동기화 완료!";
    } else {
        qDebug() << "커밋 실패:" << m_db.lastError().text();
        m_db.rollback(); // 실패 시 되돌리기
    }

}



void SqlHandler::initData() {
    if(!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.exec("SELECT name FROM customer");

    while (query.next()) {
        // 맵(딕셔너리) 구조로 만들어서 담기
        QVariant var;
        var = query.value("name").toString();
        dataName.append(var);
    }

    query.exec("SELECT * FROM item");

    while (query.next()) {
        QVariantMap map;

        map["id"] = query.value(0); //id
        map["name"]  = query.value(1); // item_name
        map["spec"]  = query.value(2); // spec
        map["price"] = query.value(3); // price
        dataProduct.append(map);
    }
}

void SqlHandler::refreshData() {
    if(!m_db.isOpen()) return;

    dataName.clear();
    dataProduct.clear();

    QSqlQuery query(m_db);
    query.exec("SELECT name FROM customer");

    while (query.next()) {
        // 맵(딕셔너리) 구조로 만들어서 담기
        QVariant var;
        var = query.value("name").toString();
        dataName.append(var);
    }

    query.exec("SELECT * FROM item");

    while (query.next()) {
        QVariantMap map;

        map["id"] = query.value(0); //id
        map["name"]  = query.value(1); // item_name
        map["spec"]  = query.value(2); // spec
        map["price"] = query.value(3); // price
        dataProduct.append(map);
    }
}

bool SqlHandler::readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae, const QVariant &supplier, const QVariant &product) {
    if(!m_db.isOpen()) return false;

    searchedResult.clear();
    gaesoo = 0;

    QStringList a;
    a <<"id"<<"gb"<<"date"<<"supplier"<<"product"<<"size"<<"price"<<"quantity"<<"gongga"<<"buga"<<"hapgye"<<"ipD1"<<"ipA1"<<"ipD2"<<"ipA2"<<"ipD3"<<"ipA3"<<"miji"<<"misu";

    QString gubun;
    if(mae) {
        gubun = "매입";
    }
    else {
        gubun = "매출";
    }

    QSqlQuery query(m_db);
    QString queryStr;
    queryStr = QString("SELECT * FROM records WHERE tr_date BETWEEN '%1' and '%2' AND gubun = '%3'").arg(startDate.toString(), endDate.toString(), gubun);
    if(supplier != "전체") {
        queryStr += QString("AND customer = '%1'").arg(supplier.toString());
    }
    if(product != "전체") {
        queryStr += QString("AND item = '%1'").arg(product.toString());
    }
    query.exec(queryStr);

    while(query.next()) {
        QVariantMap map;
        gaesoo += 1;

        for(int i=0;i<19;i++) {
            map[a[i]] = query.value(i);
        }
        searchedResult.append(map);
    }
    calcSearchedSum(startDate, endDate, mae, supplier, product);
    return true;
}


// void SqlHandler::writeExcelRecord(bool mae, const QVariant &date, const QVariant &supplier, const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &quantity) {

// }

void SqlHandler::calcSearchedSum(const QVariant &startDate, const QVariant &endDate, bool mae, const QVariant &supplier, const QVariant &product) {

    amountSum = 0;
    gonggaSum = 0;
    bugaSum = 0;
    hapgyeSum = 0;
    ipamountSum = 0;
    misuSum = 0;
    mijiSum = 0;

    QString gubun;
    if(mae) {
        gubun = "매입";
    }
    else {
        gubun = "매출";
    }

    QSqlQuery query(m_db);
    QString queryStr;
    queryStr = QString("SELECT SUM(amount), SUM(supply_val), SUM(tax_val), SUM(total_val), SUM(pay_amt1 + pay_amt2 + pay_amt3), SUM(unpaid_amt), SUM(receivable_amt) FROM records WHERE tr_date BETWEEN '%1' and '%2' AND gubun = '%3'").arg(startDate.toString(), endDate.toString(), gubun);
    if(supplier != "전체") {
        queryStr += QString("AND customer = '%1'").arg(supplier.toString());
    }
    if(product != "전체") {
        queryStr += QString("AND item = '%1'").arg(product.toString());
    }
    query.exec(queryStr);
    if(query.exec(queryStr) && query.next()) {

        amountSum = query.value(0).toInt();
        gonggaSum = query.value(1).toInt();
        bugaSum = query.value(2).toInt();
        hapgyeSum = query.value(3).toInt();
        ipamountSum = query.value(4).toInt();
        mijiSum = query.value(5).toInt();
        misuSum = query.value(6).toInt();
    }
}

void SqlHandler::monthTotalReady(const QVariant &year, const QVariant &gb, const QVariant &supplier, const QVariant &product) {
    if(!m_db.isOpen()) return;

    QString baseCondition = QString("WHERE strftime('%Y', tr_date) = '%1' AND gubun = '%2' ")
                                .arg(year.toString(), gb.toString());

    if(supplier.toString() != "전체")
        baseCondition += QString(" AND customer = '%1' ").arg(supplier.toString());
    if(product.toString() != "전체")
        baseCondition += QString(" AND item = '%1' ").arg(product.toString());

    // 2. 각각의 함수에 "어디서 가져올지(WHERE절)"를 던져줍니다.
    calcMonthTotal(baseCondition);
    calcBungiTotal(baseCondition);
    calcBangiTotal(baseCondition);

}

void SqlHandler::calcMonthTotal(QString queryStr) {
    monthTotal.clear();
    QSqlQuery query(m_db);
    QStringList a;
    a <<"num" <<"amount"<<"gongga"<<"buga"<<"hapgye"<<"miji"<<"misu";

    // 월별 합계 쿼리 조립
    QString sql = "SELECT strftime('%m', tr_date) as m, SUM(amount), SUM(supply_val), SUM(tax_val), SUM(total_val), SUM(unpaid_amt), SUM(receivable_amt)"
                  "FROM records " + queryStr + " GROUP BY m ORDER BY m;";

    if(query.exec(sql)) {
        while(query.next()) {
            QVariantMap map;

            for(int i=0;i<7;i++) {
                map[a[i]] = query.value(i);
            }
            monthTotal.append(map);
        }
    }
}

void SqlHandler::calcBungiTotal(QString queryStr) {
    bungiTotal.clear();
    QSqlQuery query(m_db);

    QStringList a;
    a <<"num" <<"amount"<<"gongga"<<"buga"<<"hapgye"<<"miji"<<"misu";

    // 분기 계산기 장착 (1~3월=1, 4~6월=2...)
    QString sql = "SELECT (strftime('%m', tr_date)-1)/3 + 1 as q, SUM(amount), SUM(supply_val), SUM(tax_val), SUM(total_val), SUM(unpaid_amt), SUM(receivable_amt)"
                  "FROM records " + queryStr + " GROUP BY q ORDER BY q;";

    if(query.exec(sql)) {
        while(query.next()) {
            QVariantMap map;

            for(int i=0;i<7;i++) {
                map[a[i]] = query.value(i);
            }
            bungiTotal.append(map);
        }
    }
}

void SqlHandler::calcBangiTotal(QString queryStr) {
    bangiTotal.clear();
    QSqlQuery query(m_db);
    QStringList a;
    a <<"num" <<"amount"<<"gongga"<<"buga"<<"hapgye"<<"miji"<<"misu";

    QString sql = "SELECT "
                  "  CASE WHEN strftime('%m', tr_date) <= '06' THEN '1' ELSE '2' END AS h, "
                  "  SUM(amount), SUM(supply_val), SUM(tax_val), SUM(total_val), SUM(unpaid_amt), SUM(receivable_amt) "
                  "FROM records "
                  + queryStr + // queryStr 끝에 공백이 있는지 확인 필수!
                  " GROUP BY h ORDER BY h;";

    if(query.exec(sql)) {
        while(query.next()) {
            QVariantMap map;

            for(int i=0;i<7;i++) {
                map[a[i]] = query.value(i);
            }
            map["gb"] = "";
            bangiTotal.append(map);
        }
    }
}

void SqlHandler::writeDataName(const QVariant &name) {
    if(!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO customer (name) VALUES (:name)");
    query.bindValue(":name", name.toString());

    if(!query.exec()) {
        qDebug() << "추가 실패:" << query.lastError().text();
    }
}

void SqlHandler::writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price) {
    if(!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("INSERT INTO item (item_name, spec, price) VALUES (:product, :size, :price)");
    query.bindValue(":product", product.toString());
    query.bindValue(":size", size.toString());
    query.bindValue(":price", price.toString());

    if(!query.exec()) {
        qDebug() << "추가 실패:" << query.lastError().text();
    }
}

void SqlHandler::editDataSupplier(const QVariant &name, const QVariant &count) {
    if(!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("UPDATE customer SET name = :name WHERE id = :id");
    query.bindValue(":name", name);
    query.bindValue(":id", count);

    if(!query.exec()) {
        qDebug() << "수정 실패:" << query.lastError().text();
    }
}

void SqlHandler::editDataProduct(const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &count) {
    if(!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("UPDATE item SET item_name = :name, spec = :size, price = :price WHERE id = :id");
    query.bindValue(":name", product);
    query.bindValue(":size", size.toString());
    query.bindValue(":price", price);
    query.bindValue(":id", count);

    if(!query.exec()) {
        qDebug() << "수정 실패:" << query.lastError().text();
    }
}

void SqlHandler::writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2, const QVariant &amount2, const QVariant &date3, const QVariant &amount3, const QVariant &row) {
    if(!m_db.isOpen()) return;

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


    if(!query.exec()) {
        qDebug() << "추가 실패:" << query.lastError().text();
    }
}

void SqlHandler::deleteDataSupplier(const QVariant &count) {
    if(!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM customer WHERE id = :id");
    query.bindValue(":id", count);

    if(!query.exec()) {
        qDebug() << "삭제 실패:" << query.lastError().text();
    }
}

void SqlHandler::deleteDataProduct(const QVariant &count) {
    if(!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM item WHERE id = :id");
    query.bindValue(":id", count);

    if(!query.exec()) {
        qDebug() << "삭제 실패:" << query.lastError().text();
    }
}

void SqlHandler::deleteRecord(const QVariant &row) {
    if(!m_db.isOpen()) return;

    QSqlQuery query(m_db);
    query.prepare("DELETE FROM records WHERE id = :id");
    query.bindValue(":id", row);

    if(!query.exec()) {
        qDebug() << "삭제 실패:" << query.lastError().text();
    }
}

void SqlHandler::writeExcelRecord(const bool &mae, const QVariant &date, const QVariant &supplier, const QVariant &product, const QVariant &size, const QVariant &price, const QVariant &quantity, const bool &tax) {
    if(!m_db.isOpen()) return;

    QString gb;
    int gongga = price.toInt()*quantity.toInt();
    int buga = 0;
    if(tax) {
        buga = gongga / 10;
    }
    if(mae) {
        gb = QString("매입");
    }
    else {
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
    query.bindValue(":hap", gongga+buga);


    if(!query.exec()) {
        qDebug() << "추가 실패:" << query.lastError().text();
    }
}

QList<QStringList> SqlHandler::readAllSqlRecord() {
    QList<QStringList> list;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM records");

    if(query.exec()) {
        while (query.next()) {
            QStringList data;
            for(int i=1;i<19;i++) {
                //qDebug() << query.value(i).toString();
                data << query.value(i).toString();
            }
            qDebug() << data;
            list.append(data);
        }
    }
    return list;
}

QList<QStringList> SqlHandler::readAllSqlItem() {
    QList<QStringList> list;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM item");

    if(query.exec()) {
        while (query.next()) {
            QStringList data;
            for(int i=1;i<4;i++) {
                //qDebug() << query.value(i).toString();
                data << query.value(i).toString();
            }
            list.append(data);
        }
    }
    return list;
}

QVariantList SqlHandler::readAllSqlCustomer() {
    QVariantList list;

    QSqlQuery query(m_db);
    query.prepare("SELECT * FROM customer");

    if(query.exec()) {
        while (query.next()) {
            QVariant data;
            //qDebug() << query.value(1);
            data = query.value(1);
            list.append(data);
        }
    }
    return list;
}

// -------------------------------------------------------------
// qml로 데이터 반환하는 함수들
// -------------------------------------------------------------
QVariantList SqlHandler::getDataName() const
{
    return dataName;
}

QVariantList SqlHandler::getDataProduct() const
{
    return dataProduct;
}

QVariantList SqlHandler::getSearchedResult() const
{
    return searchedResult;
}

QVariant SqlHandler::getAmountSum() const
{
    return amountSum;
}

QVariant SqlHandler::getGonggaSum() const
{
    return gonggaSum;
}

QVariant SqlHandler::getBugaSum() const
{
    return bugaSum;
}

QVariant SqlHandler::getHapgyeSum() const
{
    return hapgyeSum;
}

QVariant SqlHandler::getIpamountSum() const
{
    return ipamountSum;
}

QVariant SqlHandler::getMisuSum() const
{
    return misuSum;
}

QVariant SqlHandler::getMijiSum() const
{
    return mijiSum;
}

QVariantList SqlHandler::getMonthTotal() const
{
    return monthTotal;
}

QVariantList SqlHandler::getBungiTotal() const
{
    return bungiTotal;
}

QVariantList SqlHandler::getBangiTotal() const
{
    return bangiTotal;
}

int SqlHandler::getGaesoo() const
{
    return gaesoo;
}
