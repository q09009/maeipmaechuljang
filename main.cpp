#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDebug>
#include "datahandler.h"

#include <QList>

#include "xlsxdocument.h"

using namespace QXlsx;



int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    DataHandler handler(&app);

    QQmlApplicationEngine engine;

    engine.rootContext()->setContextProperty("excelData", &handler);

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
