#include "sqlDatahandler.h"
#include "excelDatahandler.h"
#include "syncManager.h"
#include <QObject>
#include <QDebug>


void SyncManager::StoE() {

    QList<QStringList> record = m_sql->readAllSqlRecord();

    if (!record.isEmpty()) {
        m_excel->syncRecord(record);
        qDebug() << "동기화 작업 완료!";
    }
    else {
        qDebug() << "Sql에 데이터가 없거나 파일을 못 찾았습니다.";
    }

    QList<QStringList> items = m_sql->readAllSqlItem();
    QVariantList customers = m_sql->readAllSqlCustomer();
    if (!items.isEmpty() && !customers.isEmpty()) {
        m_excel->syncData(customers, items);
        qDebug() << "동기화 작업 완료!";
    }
    else {
        qDebug() << "Sql에 데이터가 없거나 파일을 못 찾았습니다.";
    }
}

void SyncManager::EtoS() {
    QList<QStringList> data = m_excel->readAllExcelRecord();
    if (!data.isEmpty()) {
        m_sql->syncExcelToSql(data);
        qDebug() << "동기화 작업 완료!";
    }
    else {
        qDebug() << "엑셀에 데이터가 없거나 파일을 못 찾았습니다.";
    }

    QList<QStringList> items = m_excel->readAllExcelItem();
    QVariantList customers = m_excel->readAllExcelCustomer();
    if (!items.isEmpty() && !customers.isEmpty()) {
        m_sql->syncExcelToSqlData(customers, items);
        qDebug() << "동기화 작업 완료!";
    }
    else {
        qDebug() << "엑셀에 데이터가 없거나 파일을 못 찾았습니다.";
    }
}

void SyncManager::makeMonthlyClosing(const QVariant &year, const QVariant &month) {
    QList<QStringList> data = m_sql->readMonthlySql(year, month);
    if(!data.isEmpty()) {
        m_excel->makeMonthlyClosingExcel(data);
    }
}
