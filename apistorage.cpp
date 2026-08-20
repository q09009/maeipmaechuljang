#include "apistorage.h"

#include <QDate>
#include <QDateTime>
#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <utility>

namespace {
QString jsonDate(const QJsonValue &value) {
    return value.isNull() || value.isUndefined() ? QString() : value.toString();
}

QVariantMap recordToUiMap(const QJsonObject &obj) {
    return {
        {"id", obj["id"].toInt()},
        {"gb", obj["gubun"].toString()},
        {"date", obj["tr_date"].toString()},
        {"supplier", obj["customer"].toString()},
        {"product", obj["item"].toString()},
        {"size", obj["spec"].toString()},
        {"price", obj["price"].toVariant()},
        {"quantity", obj["amount"].toVariant()},
        {"gongga", obj["supply_val"].toVariant()},
        {"buga", obj["tax_val"].toVariant()},
        {"hapgye", obj["total_val"].toVariant()},
        {"ipD1", jsonDate(obj["pay_date1"])},
        {"ipA1", obj["pay_amt1"].toVariant()},
        {"ipD2", jsonDate(obj["pay_date2"])},
        {"ipA2", obj["pay_amt2"].toVariant()},
        {"ipD3", jsonDate(obj["pay_date3"])},
        {"ipA3", obj["pay_amt3"].toVariant()},
        {"miji", obj["unpaid_amt"].toVariant()},
        {"misu", obj["receivable_amt"].toVariant()},
    };
}
} // namespace

ApiStorage::ApiStorage(const QString &baseUrl, const QString &apiKey)
    : m_baseUrl(baseUrl.trimmed()), m_apiKey(apiKey) {
    while (m_baseUrl.endsWith('/'))
        m_baseUrl.chop(1);
}

QByteArray ApiStorage::syncRequest(const QByteArray &method, const QString &path,
                                   const QByteArray &body) {
    QNetworkRequest request(QUrl(m_baseUrl + path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");
    request.setTransferTimeout(15000);
    if (!m_apiKey.isEmpty())
        request.setRawHeader("X-API-Key", m_apiKey.toUtf8());

    QNetworkReply *reply = nullptr;
    if (method == "GET")
        reply = m_nam.get(request);
    else if (method == "POST")
        reply = m_nam.post(request, body);
    else if (method == "PUT")
        reply = m_nam.put(request, body);
    else
        reply = m_nam.sendCustomRequest(request, method, body);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const QByteArray response = reply->readAll();
    const int httpStatus = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const bool statusOk = httpStatus >= 200 && httpStatus < 300;
    m_lastRequestOk = reply->error() == QNetworkReply::NoError && statusOk;

    if (m_lastRequestOk) {
        m_lastError.clear();
    } else {
        QString detail;
        const QJsonDocument errorDoc = QJsonDocument::fromJson(response);
        if (errorDoc.isObject())
            detail = errorDoc.object().value("detail").toString();
        if (detail.isEmpty())
            detail = reply->errorString();
        m_lastError = QString("HTTP %1: %2").arg(httpStatus).arg(detail);
        qWarning() << "[API]" << method << path << "실패:" << m_lastError;
    }

    reply->deleteLater();
    return m_lastRequestOk ? response : QByteArray();
}

QByteArray ApiStorage::syncGet(const QString &path) {
    return syncRequest("GET", path);
}

QByteArray ApiStorage::syncPost(const QString &path, const QByteArray &body) {
    return syncRequest("POST", path, body);
}

QByteArray ApiStorage::syncPut(const QString &path, const QByteArray &body) {
    return syncRequest("PUT", path, body);
}

bool ApiStorage::syncDelete(const QString &path) {
    syncRequest("DELETE", path);
    return m_lastRequestOk;
}

QString ApiStorage::isoDate(const QVariant &value) {
    const QDate date = value.toDate();
    if (date.isValid())
        return date.toString(Qt::ISODate);
    const QDateTime dateTime = value.toDateTime();
    if (dateTime.isValid())
        return dateTime.date().toString(Qt::ISODate);
    const QString text = value.toString().trimmed();
    const QDate parsed = QDate::fromString(text.left(10), Qt::ISODate);
    return parsed.isValid() ? parsed.toString(Qt::ISODate) : text;
}

void ApiStorage::putNullableDate(QJsonObject &object, const QString &key, const QVariant &value) {
    const QString date = isoDate(value);
    object[key] = date.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(date);
}

bool ApiStorage::initDB() {
    syncGet("/health");
    if (!m_lastRequestOk) {
        qCritical() << "[API] 서버 연결 실패 -" << m_baseUrl << m_lastError;
        return false;
    }
    qInfo() << "[API] 서버 연결 성공 -" << m_baseUrl;
    refreshData();
    return true;
}

QList<QStringList> ApiStorage::readAllSqlRecord() {
    const QJsonArray records = QJsonDocument::fromJson(syncGet("/records")).array();
    QList<QStringList> result;
    for (const QJsonValue &value : records) {
        const QJsonObject obj = value.toObject();
        result.append({
            obj["gubun"].toString(), obj["tr_date"].toString(), obj["customer"].toString(),
            obj["item"].toString(), obj["spec"].toString(), QString::number(obj["price"].toInteger()),
            QString::number(obj["amount"].toInteger()), QString::number(obj["supply_val"].toInteger()),
            QString::number(obj["tax_val"].toInteger()), QString::number(obj["total_val"].toInteger()),
            jsonDate(obj["pay_date1"]), QString::number(obj["pay_amt1"].toInteger()),
            jsonDate(obj["pay_date2"]), QString::number(obj["pay_amt2"].toInteger()),
            jsonDate(obj["pay_date3"]), QString::number(obj["pay_amt3"].toInteger()),
            QString::number(obj["unpaid_amt"].toInteger()),
            QString::number(obj["receivable_amt"].toInteger()),
        });
    }
    return result;
}

QList<QStringList> ApiStorage::readAllSqlItem() {
    const QJsonArray items = QJsonDocument::fromJson(syncGet("/items")).array();
    QList<QStringList> result;
    for (const QJsonValue &value : items) {
        const QJsonObject obj = value.toObject();
        result.append({obj["item_name"].toString(), obj["spec"].toString(),
                       QString::number(obj["price"].toDouble())});
    }
    return result;
}

QVariantList ApiStorage::readAllSqlCustomer() {
    const QJsonArray customers = QJsonDocument::fromJson(syncGet("/customers")).array();
    QVariantList result;
    for (const QJsonValue &value : customers)
        result.append(value.toObject()["name"].toString());
    return result;
}

void ApiStorage::refreshData() {
    const QJsonArray customers = QJsonDocument::fromJson(syncGet("/customers")).array();
    m_dataName.clear();
    m_customerIds.clear();
    for (const QJsonValue &value : customers) {
        const QJsonObject obj = value.toObject();
        m_dataName.append(obj["name"].toString());
        m_customerIds.append(obj["id"].toInt());
    }

    const QJsonArray items = QJsonDocument::fromJson(syncGet("/items")).array();
    m_dataProduct.clear();
    for (const QJsonValue &value : items) {
        const QJsonObject obj = value.toObject();
        m_dataProduct.append(QVariantMap{
            {"id", obj["id"].toInt()},
            {"name", obj["item_name"].toString()},
            {"spec", obj["spec"].toString()},
            {"price", obj["price"].toDouble()},
        });
    }
}

QVariantList ApiStorage::getDataName() const { return m_dataName; }
QVariantList ApiStorage::getDataProduct() const { return m_dataProduct; }

void ApiStorage::writeDataName(const QVariant &name) {
    QJsonObject obj{{"name", name.toString().trimmed()}};
    syncPost("/customers", QJsonDocument(obj).toJson(QJsonDocument::Compact));
    if (m_lastRequestOk)
        refreshData();
}

void ApiStorage::writeDataProduct(const QVariant &product, const QVariant &size,
                                  const QVariant &price) {
    QJsonObject obj{{"item_name", product.toString().trimmed()},
                    {"spec", size.toString().trimmed()},
                    {"price", price.toDouble()}};
    syncPost("/items", QJsonDocument(obj).toJson(QJsonDocument::Compact));
    if (m_lastRequestOk)
        refreshData();
}

void ApiStorage::writeExcelRecord(const bool &mae, const QVariant &date,
                                  const QVariant &supplier, const QVariant &product,
                                  const QVariant &size, const QVariant &price,
                                  const QVariant &quantity, const bool &tax) {
    QJsonObject obj{{"gubun", mae ? "매입" : "매출"},
                    {"tr_date", isoDate(date)},
                    {"customer", supplier.toString()},
                    {"item", product.toString()},
                    {"spec", size.toString()},
                    {"price", price.toInt()},
                    {"amount", quantity.toInt()},
                    {"tax", tax}};
    syncPost("/records", QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

bool ApiStorage::readRecordRange(const QVariant &startDate, const QVariant &endDate,
                                 bool mae, const QVariant &supplier,
                                 const QVariant &product) {
    QUrlQuery query;
    query.addQueryItem("start", isoDate(startDate));
    query.addQueryItem("end", isoDate(endDate));
    query.addQueryItem("gubun", mae ? "매입" : "매출");
    if (supplier.toString() != "전체" && !supplier.toString().isEmpty())
        query.addQueryItem("customer", supplier.toString());
    if (product.toString() != "전체" && !product.toString().isEmpty())
        query.addQueryItem("item", product.toString());

    const QByteArray response = syncGet("/records/search?" + query.toString(QUrl::FullyEncoded));
    if (!m_lastRequestOk)
        return false;

    const QJsonObject root = QJsonDocument::fromJson(response).object();
    m_searchedResult.clear();
    for (const QJsonValue &value : root["records"].toArray())
        m_searchedResult.append(recordToUiMap(value.toObject()));

    m_amountSum = root["amount_sum"].toVariant();
    m_gonggaSum = root["supply_sum"].toVariant();
    m_bugaSum = root["tax_sum"].toVariant();
    m_hapgyeSum = root["total_sum"].toVariant();
    m_ipamountSum = root["ip_sum"].toVariant();
    m_misuSum = root["misu_sum"].toVariant();
    m_mijiSum = root["miji_sum"].toVariant();
    m_gaesoo = m_searchedResult.size();
    return true;
}

QVariantList ApiStorage::getSearchedResult() const { return m_searchedResult; }
QVariant ApiStorage::getAmountSum() const { return m_amountSum; }
QVariant ApiStorage::getGonggaSum() const { return m_gonggaSum; }
QVariant ApiStorage::getBugaSum() const { return m_bugaSum; }
QVariant ApiStorage::getHapgyeSum() const { return m_hapgyeSum; }
QVariant ApiStorage::getIpamountSum() const { return m_ipamountSum; }
QVariant ApiStorage::getMisuSum() const { return m_misuSum; }
QVariant ApiStorage::getMijiSum() const { return m_mijiSum; }
int ApiStorage::getGaesoo() const { return m_gaesoo; }

void ApiStorage::monthTotalReady(const QVariant &year, const QVariant &gb,
                                 const QVariant &supplier, const QVariant &product) {
    QUrlQuery query;
    query.addQueryItem("year", year.toString());
    query.addQueryItem("gubun", gb.toString());
    if (supplier.toString() != "전체" && !supplier.toString().isEmpty())
        query.addQueryItem("customer", supplier.toString());
    if (product.toString() != "전체" && !product.toString().isEmpty())
        query.addQueryItem("item", product.toString());

    const QJsonObject root = QJsonDocument::fromJson(
        syncGet("/records/monthly?" + query.toString(QUrl::FullyEncoded))).object();
    if (!m_lastRequestOk)
        return;

    auto toVariantList = [](const QJsonArray &array) {
        QVariantList result;
        for (const QJsonValue &value : array)
            result.append(value.toObject().toVariantMap());
        return result;
    };
    m_monthTotal = toVariantList(root["monthly"].toArray());
    m_bungiTotal = toVariantList(root["quarterly"].toArray());
    m_bangiTotal = toVariantList(root["halfyear"].toArray());
}

QVariantList ApiStorage::getMonthTotal() const { return m_monthTotal; }
QVariantList ApiStorage::getBungiTotal() const { return m_bungiTotal; }
QVariantList ApiStorage::getBangiTotal() const { return m_bangiTotal; }

void ApiStorage::deleteRecord(const QVariant &row) {
    syncDelete("/records/" + row.toString());
}

void ApiStorage::editDataSupplier(const QVariant &name, const QVariant &count) {
    const int index = count.toInt() - 1;
    const int customerId = (index >= 0 && index < m_customerIds.size())
                               ? m_customerIds.at(index)
                               : count.toInt();
    QJsonObject obj{{"name", name.toString().trimmed()}};
    syncPut("/customers/" + QString::number(customerId),
            QJsonDocument(obj).toJson(QJsonDocument::Compact));
    if (m_lastRequestOk)
        refreshData();
}

void ApiStorage::editDataProduct(const QVariant &product, const QVariant &size,
                                 const QVariant &price, const QVariant &count) {
    QJsonObject obj{{"item_name", product.toString().trimmed()},
                    {"spec", size.toString().trimmed()},
                    {"price", price.toDouble()}};
    syncPut("/items/" + count.toString(), QJsonDocument(obj).toJson(QJsonDocument::Compact));
    if (m_lastRequestOk)
        refreshData();
}

void ApiStorage::deleteDataSupplier(const QVariant &count) {
    const int index = count.toInt() - 1;
    const int customerId = (index >= 0 && index < m_customerIds.size())
                               ? m_customerIds.at(index)
                               : count.toInt();
    if (syncDelete("/customers/" + QString::number(customerId)))
        refreshData();
}

void ApiStorage::deleteDataProduct(const QVariant &count) {
    if (syncDelete("/items/" + count.toString()))
        refreshData();
}

void ApiStorage::syncExcelToSql(const QList<QStringList> &dataList) {
    QJsonArray records;
    for (const QStringList &row : dataList) {
        if (row.size() < 16)
            continue;
        QJsonObject obj{{"gubun", row[0]}, {"tr_date", row[1]}, {"customer", row[2]},
                        {"item", row[3]}, {"spec", row[4]}, {"price", row[5].toLongLong()},
                        {"amount", row[6].toLongLong()}, {"supply_val", row[7].toLongLong()},
                        {"tax_val", row[8].toLongLong()}, {"total_val", row[9].toLongLong()},
                        {"pay_amt1", row[11].toLongLong()}, {"pay_amt2", row[13].toLongLong()},
                        {"pay_amt3", row[15].toLongLong()}};
        putNullableDate(obj, "pay_date1", row[10]);
        putNullableDate(obj, "pay_date2", row[12]);
        putNullableDate(obj, "pay_date3", row[14]);
        records.append(obj);
    }
    QJsonObject root{{"records", records}};
    syncPost("/imports/records", QJsonDocument(root).toJson(QJsonDocument::Compact));
}

void ApiStorage::syncExcelToSqlData(const QVariantList &customers,
                                    const QList<QStringList> &items) {
    QJsonArray customerArray;
    for (const QVariant &customer : customers)
        customerArray.append(customer.toString());

    QJsonArray itemArray;
    for (const QStringList &item : items) {
        if (item.size() < 3)
            continue;
        itemArray.append(QJsonObject{{"item_name", item[0]}, {"spec", item[1]},
                                     {"price", item[2].toDouble()}});
    }
    QJsonObject root{{"customers", customerArray}, {"items", itemArray}};
    syncPost("/imports/reference-data", QJsonDocument(root).toJson(QJsonDocument::Compact));
    if (m_lastRequestOk)
        refreshData();
}

void ApiStorage::writeRecordIp(const QVariant &date1, const QVariant &amount1,
                               const QVariant &date2, const QVariant &amount2,
                               const QVariant &date3, const QVariant &amount3,
                               const QVariant &row) {
    QJsonObject obj{{"pay_amt1", amount1.toLongLong()},
                    {"pay_amt2", amount2.toLongLong()},
                    {"pay_amt3", amount3.toLongLong()}};
    putNullableDate(obj, "pay_date1", date1);
    putNullableDate(obj, "pay_date2", date2);
    putNullableDate(obj, "pay_date3", date3);
    syncPut("/records/" + row.toString() + "/payment",
            QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void ApiStorage::writeRecordIpFull(const QVariant &trDate, const QVariant &date1,
                                   const QVariant &amount1, const QVariant &date2,
                                   const QVariant &amount2, const QVariant &date3,
                                   const QVariant &amount3, const QVariant &row) {
    QJsonObject obj{{"tr_date", isoDate(trDate)},
                    {"pay_amt1", amount1.toLongLong()},
                    {"pay_amt2", amount2.toLongLong()},
                    {"pay_amt3", amount3.toLongLong()}};
    putNullableDate(obj, "pay_date1", date1);
    putNullableDate(obj, "pay_date2", date2);
    putNullableDate(obj, "pay_date3", date3);
    syncPut("/records/" + row.toString() + "/payment",
            QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

void ApiStorage::writeRecordIlgwalIpgeum(const QVariant &date, const QVariant &amount) {
    QJsonArray recordIds;
    for (const QVariant &row : std::as_const(m_searchedResult))
        recordIds.append(row.toMap().value("id").toInt());
    if (recordIds.isEmpty()) {
        m_lastError = "일괄입금 대상 전표가 없습니다";
        return;
    }
    QJsonObject obj{{"date", isoDate(date)}, {"amount", amount.toLongLong()},
                    {"record_ids", recordIds}};
    syncPost("/records/bulk-payment", QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

QList<QStringList> ApiStorage::readMonthlySql(const QVariant &year, const QVariant &month) {
    QUrlQuery query;
    query.addQueryItem("year", year.toString());
    query.addQueryItem("month", month.toString());
    const QJsonArray records = QJsonDocument::fromJson(
        syncGet("/records/monthly-detail?" + query.toString(QUrl::FullyEncoded))).array();

    QList<QStringList> result;
    for (const QJsonValue &value : records) {
        const QJsonObject obj = value.toObject();
        result.append({obj["gubun"].toString(), obj["tr_date"].toString(),
                       obj["customer"].toString(), obj["item"].toString(),
                       obj["spec"].toString(), QString::number(obj["price"].toInteger()),
                       QString::number(obj["amount"].toInteger()),
                       QString::number(obj["supply_val"].toInteger()),
                       QString::number(obj["tax_val"].toInteger()),
                       QString::number(obj["total_val"].toInteger())});
    }
    return result;
}

bool ApiStorage::backupDB() {
    // PostgreSQL 백업은 FastAPI 서버에서 수행한다.
    return true;
}

void ApiStorage::cleanOldBackups() {}
void ApiStorage::cleanOldLogs() {}
