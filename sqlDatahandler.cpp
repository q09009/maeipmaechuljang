#include "sqlDatahandler.h"

#include "apistorage.h"
#include "datatransfer.h"
#include "sqlitestorage.h"

#include <QDebug>
#include <QFutureWatcher>
#include <QSettings>
#include <QTimer>
#include <QtConcurrent>

SqlHandler::SqlHandler(QObject *parent) : QObject(parent) {
    QSettings settings("MaeipMaechuljang", "DB");
    settings.remove("db/host");
    settings.remove("db/port");
    settings.remove("db/database");
    settings.remove("db/user");
    settings.remove("db/password");
    m_baseUrl = settings.value("db/baseUrl", m_baseUrl).toString();
    m_apiKey = settings.value("db/apiKey").toString();
    int savedMode = settings.value("db/mode", static_cast<int>(DbMode::Sqlite)).toInt();

    if (savedMode == static_cast<int>(DbMode::Api)) {
        setDbMode(DbMode::Api, m_baseUrl, m_apiKey);
    } else {
        // 더 이상 지원하지 않는 저장 모드 값은 SQLite로 복구한다.
        setDbMode(DbMode::Sqlite);
    }

    QTimer *backupTimer = new QTimer(this);
    connect(backupTimer, &QTimer::timeout, this, &SqlHandler::backupDB);
    backupTimer->start(1000 * 60 * 60 * 4);

    qDebug() << "자동 백업 타이머 가동: 4시간 주기";
}

SqlHandler::~SqlHandler() = default;

void SqlHandler::setDbMode(DbMode mode, const QString &baseUrl, const QString &apiKey) {
    m_mode = mode;
    if (mode == DbMode::Api) {
        m_baseUrl = baseUrl.isEmpty() ? m_baseUrl : baseUrl;
        m_apiKey = apiKey;
        m_storage = std::make_unique<ApiStorage>(m_baseUrl, m_apiKey);

        QSettings settings("MaeipMaechuljang", "DB");
        settings.setValue("db/mode",    static_cast<int>(DbMode::Api));
        settings.setValue("db/baseUrl", m_baseUrl);
        settings.setValue("db/apiKey",  m_apiKey);
    } else {
        m_storage = std::make_unique<SqliteStorage>();

        QSettings settings("MaeipMaechuljang", "DB");
        settings.setValue("db/mode", static_cast<int>(DbMode::Sqlite));
    }
}

QVariantMap SqlHandler::getDbConfig() const {
    return QVariantMap{
        {"mode",     static_cast<int>(m_mode)},
        {"baseUrl",  m_baseUrl},
        {"apiKey",   m_apiKey},
    };
}

bool SqlHandler::initDB() {
    if (m_storage) {
        bool ok = m_storage->initDB();
        if (!ok) {
            if (auto *api = dynamic_cast<ApiStorage *>(m_storage.get()))
                m_lastError = api->lastError();
            else
                m_lastError = "DB 초기화 실패";
        } else {
            m_lastError.clear();
        }
        return ok;
    }
    return false;
}

QString SqlHandler::lastError() const {
    return m_lastError;
}

void SqlHandler::startDataTransfer(TransferDirection direction) {
    if (m_transferInProgress) {
        emit dataTransferFinished(QVariantMap{
            {"success", false},
            {"message", "이미 데이터 이전 작업이 진행 중입니다"},
        });
        return;
    }
    if (m_baseUrl.trimmed().isEmpty() || m_apiKey.isEmpty()) {
        emit dataTransferFinished(QVariantMap{
            {"success", false},
            {"message", "FastAPI 서버 주소와 API 키를 먼저 저장해주세요"},
        });
        return;
    }

    m_transferInProgress = true;
    emit transferInProgressChanged();
    const bool sqliteToApi = direction == TransferDirection::SqliteToApi;
    const QString baseUrl = m_baseUrl;
    const QString apiKey = m_apiKey;
    auto *watcher = new QFutureWatcher<QVariantMap>(this);
    connect(watcher, &QFutureWatcher<QVariantMap>::finished, this, [this, watcher]() {
        const QVariantMap result = watcher->result();
        watcher->deleteLater();
        m_transferInProgress = false;
        emit transferInProgressChanged();
        emit dataTransferFinished(result);
    });
    watcher->setFuture(QtConcurrent::run([sqliteToApi, baseUrl, apiKey]() {
        return DataTransfer::run(sqliteToApi, baseUrl, apiKey);
    }));
}

void SqlHandler::syncExcelToSql(const QList<QStringList> &dataList) {
    if (m_storage) {
        m_storage->syncExcelToSql(dataList);
    }
}

void SqlHandler::syncExcelToSqlData(const QVariantList &customers, const QList<QStringList> &items) {
    if (m_storage) {
        m_storage->syncExcelToSqlData(customers, items);
    }
}

QList<QStringList> SqlHandler::readAllSqlRecord() {
    return m_storage ? m_storage->readAllSqlRecord() : QList<QStringList>();
}

QList<QStringList> SqlHandler::readAllSqlItem() {
    return m_storage ? m_storage->readAllSqlItem() : QList<QStringList>();
}

QVariantList SqlHandler::readAllSqlCustomer() {
    return m_storage ? m_storage->readAllSqlCustomer() : QVariantList();
}

void SqlHandler::refreshData() {
    if (m_storage) {
        m_storage->refreshData();
    }
}

QVariantList SqlHandler::getDataName() const {
    return m_storage ? m_storage->getDataName() : QVariantList();
}

QVariantList SqlHandler::getDataProduct() const {
    return m_storage ? m_storage->getDataProduct() : QVariantList();
}

void SqlHandler::writeDataName(const QVariant &name) {
    if (m_storage) {
        m_storage->writeDataName(name);
    }
}

void SqlHandler::writeDataProduct(const QVariant &product, const QVariant &size, const QVariant &price) {
    if (m_storage) {
        m_storage->writeDataProduct(product, size, price);
    }
}

void SqlHandler::writeExcelRecord(const bool &mae, const QVariant &date, const QVariant &supplier,
                                  const QVariant &product, const QVariant &size, const QVariant &price,
                                  const QVariant &quantity, const bool &tax) {
    if (m_storage) {
        m_storage->writeExcelRecord(mae, date, supplier, product, size, price, quantity, tax);
    }
}

void SqlHandler::writeRecordIp(const QVariant &date1, const QVariant &amount1, const QVariant &date2,
                               const QVariant &amount2, const QVariant &date3, const QVariant &amount3,
                               const QVariant &row) {
    if (m_storage) {
        m_storage->writeRecordIp(date1, amount1, date2, amount2, date3, amount3, row);
    }
}

void SqlHandler::writeRecordIpFull(const QVariant &trDate, const QVariant &date1, const QVariant &amount1,
                                   const QVariant &date2, const QVariant &amount2, const QVariant &date3,
                                   const QVariant &amount3, const QVariant &row) {
    if (m_storage) {
        m_storage->writeRecordIpFull(trDate, date1, amount1, date2, amount2, date3, amount3, row);
    }
}

void SqlHandler::writeRecordIlgwalIpgeum(const QVariant &date, const QVariant &amount) {
    if (m_storage) {
        m_storage->writeRecordIlgwalIpgeum(date, amount);
    }
}

bool SqlHandler::readRecordRange(const QVariant &startDate, const QVariant &endDate, bool mae,
                                 const QVariant &supplier, const QVariant &product) {
    return m_storage ? m_storage->readRecordRange(startDate, endDate, mae, supplier, product) : false;
}

QVariantList SqlHandler::getSearchedResult() const {
    return m_storage ? m_storage->getSearchedResult() : QVariantList();
}

QVariant SqlHandler::getAmountSum() const {
    return m_storage ? m_storage->getAmountSum() : QVariant();
}

QVariant SqlHandler::getGonggaSum() const {
    return m_storage ? m_storage->getGonggaSum() : QVariant();
}

QVariant SqlHandler::getBugaSum() const {
    return m_storage ? m_storage->getBugaSum() : QVariant();
}

QVariant SqlHandler::getHapgyeSum() const {
    return m_storage ? m_storage->getHapgyeSum() : QVariant();
}

QVariant SqlHandler::getIpamountSum() const {
    return m_storage ? m_storage->getIpamountSum() : QVariant();
}

QVariant SqlHandler::getMisuSum() const {
    return m_storage ? m_storage->getMisuSum() : QVariant();
}

QVariant SqlHandler::getMijiSum() const {
    return m_storage ? m_storage->getMijiSum() : QVariant();
}

int SqlHandler::getGaesoo() const {
    return m_storage ? m_storage->getGaesoo() : 0;
}

void SqlHandler::monthTotalReady(const QVariant &year, const QVariant &gb, const QVariant &supplier,
                                 const QVariant &product) {
    if (m_storage) {
        m_storage->monthTotalReady(year, gb, supplier, product);
    }
}

QVariantList SqlHandler::getMonthTotal() const {
    return m_storage ? m_storage->getMonthTotal() : QVariantList();
}

QVariantList SqlHandler::getBungiTotal() const {
    return m_storage ? m_storage->getBungiTotal() : QVariantList();
}

QVariantList SqlHandler::getBangiTotal() const {
    return m_storage ? m_storage->getBangiTotal() : QVariantList();
}

void SqlHandler::deleteRecord(const QVariant &row) {
    if (m_storage) {
        m_storage->deleteRecord(row);
    }
}

void SqlHandler::editDataSupplier(const QVariant &name, const QVariant &count) {
    if (m_storage) {
        m_storage->editDataSupplier(name, count);
    }
}

void SqlHandler::editDataProduct(const QVariant &product, const QVariant &size, const QVariant &price,
                                 const QVariant &count) {
    if (m_storage) {
        m_storage->editDataProduct(product, size, price, count);
    }
}

void SqlHandler::deleteDataSupplier(const QVariant &count) {
    if (m_storage) {
        m_storage->deleteDataSupplier(count);
    }
}

void SqlHandler::deleteDataProduct(const QVariant &count) {
    if (m_storage) {
        m_storage->deleteDataProduct(count);
    }
}

QList<QStringList> SqlHandler::readMonthlySql(const QVariant &year, const QVariant &month) {
    return m_storage ? m_storage->readMonthlySql(year, month) : QList<QStringList>();
}

bool SqlHandler::backupDB() {
    return m_storage ? m_storage->backupDB() : false;
}

void SqlHandler::cleanOldBackups() {
    if (m_storage) {
        m_storage->cleanOldBackups();
    }
}

void SqlHandler::cleanOldLogs() {
    if (m_storage) {
        m_storage->cleanOldLogs();
    }
}
