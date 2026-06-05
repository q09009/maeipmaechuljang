#include "sqlDatahandler.h"

#include "psqlstorage.h"
#include "sqlitestorage.h"

#include <QDebug>
#include <QTimer>

SqlHandler::SqlHandler(QObject *parent) : QObject(parent) {
    setDbMode(DbMode::Sqlite);

    QTimer *backupTimer = new QTimer(this);
    connect(backupTimer, &QTimer::timeout, this, &SqlHandler::backupDB);
    backupTimer->start(1000 * 60 * 60 * 4);

    qDebug() << "자동 백업 타이머 가동: 4시간 주기";
}

SqlHandler::~SqlHandler() = default;

void SqlHandler::setDbMode(DbMode mode, const QString &host, int port, const QString &database,
                           const QString &user, const QString &password) {
    if (mode == m_mode && m_storage) {
        if (mode == DbMode::Postgres) {
            auto *psql = dynamic_cast<PsqlStorage *>(m_storage.get());
            if (psql) {
                PsqlConfig config;
                if (!host.isEmpty()) config.host = host;
                if (port > 0) config.port = port;
                if (!database.isEmpty()) config.database = database;
                if (!user.isEmpty()) config.user = user;
                config.password = password;
                psql->setConfig(config);
            }
        }
        return;
    }

    m_mode = mode;
    if (mode == DbMode::Postgres) {
        PsqlConfig config;
        if (!host.isEmpty()) config.host = host;
        if (port > 0) config.port = port;
        if (!database.isEmpty()) config.database = database;
        if (!user.isEmpty()) config.user = user;
        config.password = password;
        m_storage = std::make_unique<PsqlStorage>(config);
    } else {
        m_storage = std::make_unique<SqliteStorage>();
    }
}

void SqlHandler::initDB() {
    if (m_storage) {
        m_storage->initDB();
    }
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
