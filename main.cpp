#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
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
                  .arg(time, (type == QtInfoMsg ? "INFO" : "OTHER"), file)
                  .arg(context.line).arg(msg) << Qt::endl;

        outFile.close();
    }
}

const QString CURRENT_VERSION = "v1.4.1";

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QNetworkAccessManager manager;
    QUrl updateUrl("https://api.github.com/repos/q09009/maeipmaechuljang/releases/latest");
    QNetworkRequest request(updateUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MyUpdater");

    // 2. 업데이트 체크 (동기 방식처럼 보이지만 이벤트 루프 활용)
    QNetworkReply *reply = manager.get(request);

    // 업데이트 체크 완료될 때까지 잠시 대기하는 이벤트 루프
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString latestVersion = doc.object().value("tag_name").toString();
        //qDebug() << latestVersion;

        if (latestVersion != CURRENT_VERSION) {
            // 3. 업데이트 발견! 업데이터 실행하고 바로 종료
            // (여기서 실제 다운로드 로직을 넣거나, 업데이터가 다운로드까지 하게 시키면 됨)
            // if (QProcess::startDetached("./updater.exe")) {
            //     return 0; // 메인 앱 실행 안 하고 바로 끝냄
            // }
            qDebug() << "업데이트 발견!";
        }
    }
    reply->deleteLater();

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
    sqlHandler.cleanOldLogs();

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
