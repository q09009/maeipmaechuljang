#include "transfersnapshot.h"

#include <QCryptographicHash>
#include <QDate>
#include <QJsonArray>
#include <QJsonValue>

#include <algorithm>
#include <cmath>

namespace {
enum class FieldKind { Text, Integer, Decimal, Date };

struct FieldSpec {
    const char *name;
    FieldKind kind;
    bool nullable = false;
};

const QList<FieldSpec> customerFields{{"name", FieldKind::Text},
                                      {"balance", FieldKind::Integer}};
const QList<FieldSpec> itemFields{{"item_name", FieldKind::Text},
                                  {"spec", FieldKind::Text},
                                  {"price", FieldKind::Decimal}};
const QList<FieldSpec> recordFields{
    {"gubun", FieldKind::Text},       {"tr_date", FieldKind::Date},
    {"customer", FieldKind::Text},    {"item", FieldKind::Text},
    {"spec", FieldKind::Text},        {"price", FieldKind::Integer},
    {"amount", FieldKind::Integer},   {"supply_val", FieldKind::Integer},
    {"tax_val", FieldKind::Integer},  {"total_val", FieldKind::Integer},
    {"pay_date1", FieldKind::Date, true}, {"pay_amt1", FieldKind::Integer},
    {"pay_date2", FieldKind::Date, true}, {"pay_amt2", FieldKind::Integer},
    {"pay_date3", FieldKind::Date, true}, {"pay_amt3", FieldKind::Integer},
};

QString integerText(const QJsonValue &value, bool *ok = nullptr) {
    bool valid = false;
    qint64 number = 0;
    if (value.isString()) {
        number = value.toString().toLongLong(&valid);
    } else if (value.isDouble()) {
        const double raw = value.toDouble();
        valid = std::isfinite(raw) && std::floor(raw) == raw;
        if (valid)
            number = value.toInteger();
    }
    if (ok)
        *ok = valid;
    return valid ? QString::number(number) : QString();
}

QString decimalText(const QJsonValue &value, bool *ok = nullptr) {
    bool valid = false;
    const QString raw = value.isString() ? value.toString() : QString::number(value.toDouble(), 'g', 17);
    const double number = raw.toDouble(&valid);
    valid = valid && std::isfinite(number);
    QString result;
    if (valid) {
        result = QString::number(number, 'f', 8);
        while (result.contains('.') && result.endsWith('0'))
            result.chop(1);
        if (result.endsWith('.'))
            result.chop(1);
        if (result == "-0")
            result = "0";
    }
    if (ok)
        *ok = valid;
    return result;
}

QString normalizedValue(const QJsonValue &value, const FieldSpec &field, bool *ok) {
    if ((value.isNull() || value.isUndefined()) && field.nullable) {
        *ok = true;
        return {};
    }
    switch (field.kind) {
    case FieldKind::Text:
        *ok = value.isString();
        return value.toString();
    case FieldKind::Integer:
        return integerText(value, ok);
    case FieldKind::Decimal:
        return decimalText(value, ok);
    case FieldKind::Date: {
        const QString date = value.toString();
        *ok = value.isString() && QDate::fromString(date, Qt::ISODate).isValid();
        return date;
    }
    }
    *ok = false;
    return {};
}

bool validateRows(const QJsonArray &rows, const QList<FieldSpec> &fields,
                  const QString &section, QString *error) {
    for (qsizetype rowIndex = 0; rowIndex < rows.size(); ++rowIndex) {
        if (!rows.at(rowIndex).isObject()) {
            if (error)
                *error = QString("%1[%2] 항목이 객체가 아닙니다").arg(section).arg(rowIndex);
            return false;
        }
        const QJsonObject row = rows.at(rowIndex).toObject();
        const QString rowLabel = row.contains("source_id")
                                     ? QString("%1[%2, id=%3]")
                                           .arg(section).arg(rowIndex)
                                           .arg(row.value("source_id").toInteger())
                                     : QString("%1[%2]").arg(section).arg(rowIndex);
        for (const FieldSpec &field : fields) {
            bool ok = false;
            const QString value = normalizedValue(row.value(field.name), field, &ok);
            if (!ok || (!field.nullable && field.kind == FieldKind::Text && value.isEmpty())) {
                if (error) {
                    *error = QString("%1.%2 값이 올바르지 않습니다")
                                 .arg(rowLabel).arg(field.name);
                }
                return false;
            }
        }
    }
    return true;
}

QList<QByteArray> canonicalRows(const QJsonArray &rows, const QList<FieldSpec> &fields) {
    QList<QByteArray> result;
    result.reserve(rows.size());
    for (const QJsonValue &value : rows) {
        const QJsonObject row = value.toObject();
        QByteArray encoded;
        for (const FieldSpec &field : fields) {
            bool ok = false;
            const QByteArray part = normalizedValue(row.value(field.name), field, &ok).toUtf8();
            encoded += QByteArray::number(part.size()) + ':' + part + ';';
        }
        result.append(encoded);
    }
    std::sort(result.begin(), result.end());
    return result;
}

void addSection(QCryptographicHash &hash, const QByteArray &name,
                const QJsonArray &rows, const QList<FieldSpec> &fields) {
    const QList<QByteArray> canonical = canonicalRows(rows, fields);
    hash.addData(name);
    hash.addData(QByteArray::number(canonical.size()));
    hash.addData("\n");
    for (const QByteArray &row : canonical) {
        hash.addData(row);
        hash.addData("\n");
    }
}
} // namespace

namespace TransferSnapshot {

bool validate(const QJsonObject &snapshot, QString *error) {
    if (snapshot.value("version").toInt() != 1) {
        if (error)
            *error = "지원하지 않는 데이터 스냅샷 버전입니다";
        return false;
    }
    for (const char *section : {"customers", "items", "records"}) {
        if (!snapshot.value(section).isArray()) {
            if (error)
                *error = QString("%1 배열이 없습니다").arg(section);
            return false;
        }
    }
    if (!validateRows(snapshot.value("customers").toArray(), customerFields, "customers", error))
        return false;
    if (!validateRows(snapshot.value("items").toArray(), itemFields, "items", error))
        return false;
    if (!validateRows(snapshot.value("records").toArray(), recordFields, "records", error))
        return false;
    return true;
}

QByteArray digest(const QJsonObject &snapshot) {
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData("maeipmaechuljang-transfer-v1\n");
    addSection(hash, "customers", snapshot.value("customers").toArray(), customerFields);
    addSection(hash, "items", snapshot.value("items").toArray(), itemFields);
    addSection(hash, "records", snapshot.value("records").toArray(), recordFields);
    return hash.result().toHex();
}

QVariantMap counts(const QJsonObject &snapshot) {
    return {
        {"customers", snapshot.value("customers").toArray().size()},
        {"items", snapshot.value("items").toArray().size()},
        {"records", snapshot.value("records").toArray().size()},
    };
}

} // namespace TransferSnapshot
