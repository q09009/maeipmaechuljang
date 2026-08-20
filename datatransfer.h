#ifndef DATATRANSFER_H
#define DATATRANSFER_H

#include <QString>
#include <QVariantMap>

class DataTransfer {
public:
    static QVariantMap run(bool sqliteToApi, const QString &baseUrl, const QString &apiKey);
};

#endif // DATATRANSFER_H
