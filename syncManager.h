#include "excelDatahandler.h"
#include "sqlDatahandler.h"
#include <QObject>
#include <QDebug>

#ifndef SYNCMANAGER_H
#define SYNCMANAGER_H

class SyncManager : public QObject {
    Q_OBJECT

public:
    explicit SyncManager(QObject *parent = nullptr) : QObject(parent) {}
    void setHandlers(SqlHandler* sql, DataHandler* excel) {
        m_sql = sql;
        m_excel = excel;
    }
    Q_INVOKABLE void StoE();
    Q_INVOKABLE void EtoS();

    Q_INVOKABLE void makeMonthlyClosing(const QVariant &year, const QVariant &month);


private:
    SqlHandler* m_sql = nullptr;
    DataHandler* m_excel = nullptr;
};

#endif // SYNCMANAGER_H
