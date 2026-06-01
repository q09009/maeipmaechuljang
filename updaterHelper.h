#pragma once
#include <QObject>
#include <QProcess>
#include <QCoreApplication>

class UpdaterHelper : public QObject {
    Q_OBJECT
public:
    explicit UpdaterHelper(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void launchUpdater(const QString &currentVersion) {
        QString appDir = QCoreApplication::applicationDirPath();
        QString updaterPath = appDir + "/updater.exe";
        QProcess::startDetached(updaterPath, {"-v", currentVersion});
        QCoreApplication::quit();
    }
};
