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
#include <QList>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QTextStream>
#include <QFileInfo>

#include "excelDatahandler.h"
#include "sqlDatahandler.h"
#include "syncManager.h"
#include "xlsxdocument.h"
#include "updaterHelper.h"

using namespace QXlsx;

// [로그 출력 함수] 기존 유지
void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    if (msg.contains("The current style does not support") ||
        msg.contains("Cannot read property 'width' of null")) {
        return;
    }

    QDir().mkdir("logs");
    QString fileName = QString("logs/log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd"));

    QFile outFile(fileName);
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream ts(&outFile);
        ts.setEncoding(QStringConverter::Utf8);

        QString time = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        QString file = QFileInfo(context.file).fileName();

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

    // [깃허브 버전 체크 로직] 기존 유지
    QNetworkAccessManager manager;
    QUrl updateUrl("https://api.github.com/repos/q09009/maeipmaechuljang/releases/latest");
    QNetworkRequest request(updateUrl);
    request.setHeader(QNetworkRequest::UserAgentHeader, "MyUpdater");

    QString latestVersionStr = CURRENT_VERSION;
    bool isUpdateAvailable = false;

    auto normalizeVersion = [](QString v) {
        v = v.trimmed();
        if (v.startsWith('v', Qt::CaseInsensitive)) v.remove(0, 1);
        return v;
    };

    auto isNewerThanCurrent = [&](const QString &latest, const QString &current) {
        const QString l = normalizeVersion(latest);
        const QString c = normalizeVersion(current);
        const QStringList lp = l.split('.');
        const QStringList cp = c.split('.');
        const int n = qMax(lp.size(), cp.size());

        for (int i = 0; i < n; ++i) {
            const int lv = (i < lp.size()) ? lp[i].toInt() : 0;
            const int cv = (i < cp.size()) ? cp[i].toInt() : 0;
            if (lv > cv) return true;
            if (lv < cv) return false;
        }
        return false;
    };

    QNetworkReply *reply = manager.get(request);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        latestVersionStr = doc.object().value("tag_name").toString().trimmed();
        if (latestVersionStr.isEmpty())
            latestVersionStr = CURRENT_VERSION;

        isUpdateAvailable = isNewerThanCurrent(latestVersionStr, CURRENT_VERSION);
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

    // 👑 꼬여있던 updaterHelperObj 쓰레기 코드는 제거하고 순정 헬퍼 인스턴스만 생성!
    UpdaterHelper updaterHelper(&app);

    // QML 컨텍스트 주입 (초깔끔)
    engine.rootContext()->setContextProperty("excelData", &excelhandler);
    engine.rootContext()->setContextProperty("sqlData", &sqlHandler);
    engine.rootContext()->setContextProperty("sync", &syncManager);
    engine.rootContext()->setContextProperty("isUpdateAvailable", isUpdateAvailable);
    engine.rootContext()->setContextProperty("latestVersionStr", latestVersionStr);
    engine.rootContext()->setContextProperty("updaterHelper", &updaterHelper); // 여기에 정확히 바인딩됨
    engine.rootContext()->setContextProperty("CURRENT_VERSION", CURRENT_VERSION);

    sqlHandler.cleanOldBackups();
    sqlHandler.cleanOldLogs();

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule("maeipmaechuljang", "Main");

    QObject::connect(&app, &QCoreApplication::aboutToQuit, [&sqlHandler]() {
        qInfo() << "앱 종료 감지: 최종 백업을 시작합니다.";
        sqlHandler.backupDB();
    });

    return app.exec();
}
