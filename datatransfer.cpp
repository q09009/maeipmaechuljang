#include "datatransfer.h"

#include "apistorage.h"
#include "isqlstorage.h"
#include "sqlitestorage.h"
#include "transfersnapshot.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

#include <memory>

namespace {
QVariantMap failure(const QString &message, const QString &sourceBackup = QString(),
                    const QString &targetBackup = QString()) {
    return {
        {"success", false},
        {"message", message},
        {"sourceBackup", sourceBackup},
        {"targetBackup", targetBackup},
    };
}
} // namespace

QVariantMap DataTransfer::run(bool sqliteToApi, const QString &baseUrl, const QString &apiKey) {
    const QString connectionName = "data_transfer_" + QUuid::createUuid().toString(QUuid::Id128);
    SqliteStorage sqlite(connectionName);
    ApiStorage api(baseUrl, apiKey);

    QString error;
    if (!sqlite.initDB())
        return failure("SQLite 데이터베이스를 열지 못했습니다");
    if (!api.initDB())
        return failure("FastAPI 연결 실패: " + api.lastError());

    ISqlStorage *source = sqliteToApi ? static_cast<ISqlStorage *>(&sqlite)
                                      : static_cast<ISqlStorage *>(&api);
    ISqlStorage *target = sqliteToApi ? static_cast<ISqlStorage *>(&api)
                                      : static_cast<ISqlStorage *>(&sqlite);

    QString sourceBackup;
    if (!source->createTransferBackup(&sourceBackup, &error))
        return failure("원본 백업 실패: " + error);

    const QJsonObject sourceSnapshot = source->exportTransferSnapshot(&error);
    if (!error.isEmpty())
        return failure("원본 데이터 읽기 실패: " + error, sourceBackup);
    if (!TransferSnapshot::validate(sourceSnapshot, &error))
        return failure("원본 데이터 검증 실패: " + error, sourceBackup);

    const QByteArray sourceDigest = TransferSnapshot::digest(sourceSnapshot);
    const QVariantMap sourceCounts = TransferSnapshot::counts(sourceSnapshot);

    QString targetBackup;
    if (!target->replaceTransferSnapshot(sourceSnapshot, &targetBackup, &error)) {
        return failure("대상 데이터 교체 실패: " + error, sourceBackup, targetBackup);
    }

    error.clear();
    const QJsonObject targetSnapshot = target->exportTransferSnapshot(&error);
    if (!error.isEmpty())
        return failure("이전 후 대상 데이터 읽기 실패: " + error, sourceBackup, targetBackup);
    if (!TransferSnapshot::validate(targetSnapshot, &error))
        return failure("이전 후 대상 데이터 검증 실패: " + error, sourceBackup, targetBackup);

    const QByteArray targetDigest = TransferSnapshot::digest(targetSnapshot);
    const QVariantMap targetCounts = TransferSnapshot::counts(targetSnapshot);
    if (sourceDigest != targetDigest || sourceCounts != targetCounts) {
        return failure(
            QString("이전 후 검증 불일치 (원본 %1 / 대상 %2)")
                .arg(QString::fromLatin1(sourceDigest), QString::fromLatin1(targetDigest)),
            sourceBackup, targetBackup);
    }

    return {
        {"success", true},
        {"message", sqliteToApi ? "SQLite → PostgreSQL 이전 및 검증 완료"
                                  : "PostgreSQL → SQLite 이전 및 검증 완료"},
        {"sourceBackup", sourceBackup},
        {"targetBackup", targetBackup},
        {"digest", QString::fromLatin1(sourceDigest)},
        {"customers", sourceCounts.value("customers")},
        {"items", sourceCounts.value("items")},
        {"records", sourceCounts.value("records")},
    };
}

QVariantMap DataTransfer::reset(bool resetApi, const QString &baseUrl, const QString &apiKey) {
    const QString connectionName = "database_reset_" + QUuid::createUuid().toString(QUuid::Id128);
    std::unique_ptr<ISqlStorage> target;
    if (resetApi)
        target = std::make_unique<ApiStorage>(baseUrl, apiKey);
    else
        target = std::make_unique<SqliteStorage>(connectionName);

    if (!target->initDB()) {
        if (resetApi) {
            const auto *api = static_cast<ApiStorage *>(target.get());
            return failure("FastAPI 연결 실패: " + api->lastError());
        }
        return failure("SQLite 데이터베이스를 열지 못했습니다");
    }

    const QJsonObject emptySnapshot{
        {"version", 1},
        {"customers", QJsonArray{}},
        {"items", QJsonArray{}},
        {"records", QJsonArray{}},
    };

    QString error;
    QString targetBackup;
    if (!target->replaceTransferSnapshot(emptySnapshot, &targetBackup, &error))
        return failure("초기화 실패: " + error, QString(), targetBackup);

    const QJsonObject actual = target->exportTransferSnapshot(&error);
    if (!error.isEmpty())
        return failure("초기화 후 데이터 읽기 실패: " + error, QString(), targetBackup);
    if (!TransferSnapshot::validate(actual, &error))
        return failure("초기화 후 데이터 검증 실패: " + error, QString(), targetBackup);

    const QVariantMap counts = TransferSnapshot::counts(actual);
    if (counts.value("customers").toInt() != 0
        || counts.value("items").toInt() != 0
        || counts.value("records").toInt() != 0
        || TransferSnapshot::digest(actual) != TransferSnapshot::digest(emptySnapshot)) {
        return failure("초기화 후 검증 결과가 0건이 아닙니다", QString(), targetBackup);
    }

    return {
        {"success", true},
        {"message", resetApi ? "PostgreSQL 데이터 초기화 및 검증 완료"
                               : "SQLite 데이터 초기화 및 검증 완료"},
        {"targetBackup", targetBackup},
        {"customers", 0},
        {"items", 0},
        {"records", 0},
    };
}
