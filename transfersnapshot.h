#ifndef TRANSFERSNAPSHOT_H
#define TRANSFERSNAPSHOT_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVariantMap>

namespace TransferSnapshot {

bool validate(const QJsonObject &snapshot, QString *error = nullptr);
QByteArray digest(const QJsonObject &snapshot);
QVariantMap counts(const QJsonObject &snapshot);

} // namespace TransferSnapshot

#endif // TRANSFERSNAPSHOT_H
