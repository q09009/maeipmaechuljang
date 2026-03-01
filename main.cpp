#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include "excelDatahandler.h"
#include "sqlDatahandler.h"
#include "syncManager.h"


#include <QList>

#include "xlsxdocument.h"

using namespace QXlsx;

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {

    if (msg.contains("The current style does not support") ||
        msg.contains("Cannot read property 'width' of null")) {
        return; // 함수 바로 종료
    }

    QDir().mkdir("logs");
    QString fileName = QString("logs/log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"));

    QFile outFile(fileName);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&outFile);
        ts.setEncoding(QStringConverter::Utf8);

        QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        QString file = QFileInfo(context.file).fileName();

        // 포맷: [시간] [종류] (파일명:라인) 메시지
        ts << QString("[%1] [%2] (%3:%4) - %5")
                  .arg(time, (type == QtDebugMsg ? "DEBUG" : "ERROR"), file)
                  .arg(context.line).arg(msg) << Qt::endl;

        outFile.close();
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qInstallMessageHandler(myMessageOutput);
    qInfo() << "-------------------------------------------";
    qInfo() << "프로그램 실행됨 (로그 기록 시작)";
    qInfo() << "-------------------------------------------";

    DataHandler excelhandler(&app);
    SqlHandler sqlHandler(&app);
    SyncManager syncManager(&app);
    syncManager.setHandlers(&sqlHandler, &excelhandler);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("excelData", &excelhandler);
    engine.rootContext()->setContextProperty("sqlData", &sqlHandler);
    engine.rootContext()->setContextProperty("sync", &syncManager);

    sqlHandler.cleanOldBackups();

    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("maeipmaechuljang", "Main");

    // 앱이 종료되기 직전에 sqlHandler의 backupDB를 실행해라!
    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&sqlHandler]() {
        qInfo() << "앱 종료 감지: 최종 백업을 시작합니다.";
        sqlHandler.backupDB();
    });

    return app.exec();
}
