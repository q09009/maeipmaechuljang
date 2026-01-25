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



int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    DataHandler excelhandler(&app);
    SqlHandler sqlHandler(&app);
    SyncManager syncManager(&app);
    syncManager.setHandlers(&sqlHandler, &excelhandler);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("excelData", &excelhandler);
    engine.rootContext()->setContextProperty("sqlData", &sqlHandler);
    engine.rootContext()->setContextProperty("sync", &syncManager);

    const QUrl url(QStringLiteral("qrc:/main.qml"));

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("maeipmaechuljang", "Main");

    return app.exec();
}
