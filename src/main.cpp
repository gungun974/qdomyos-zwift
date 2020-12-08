#include <QApplication>
#include <QStyleFactory>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // getuid
#include <QStandardPaths>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QOperatingSystemVersion>
#include "virtualtreadmill.h"
#include "domyostreadmill.h"
#include "bluetooth.h"
#include "mainwindow.h"
#include "homeform.h"

#ifdef Q_OS_ANDROID
#include <QtAndroid>
#include "keepawakehelper.h"
#endif

#ifdef Q_OS_MACOS
#include "macos/lockscreen.h"
#endif

bool nologs = false;
bool noWriteResistance = false;
bool noHeartService = true;
bool noConsole = false;
bool onlyVirtualBike = false;
bool onlyVirtualTreadmill = false;
bool testResistance = false;
bool forceQml = false;
bool miles = false;
QString trainProgram;
QString deviceName = "";
uint32_t pollDeviceTime = 200;
uint8_t bikeResistanceOffset = 4;
uint8_t bikeResistanceGain = 1;
static QString logfilename = "debug-" + QDateTime::currentDateTime().toString().replace(":", "_") + ".log";

QCoreApplication* createApplication(int &argc, char *argv[])
{
    bool nogui = false;

    for (int i = 1; i < argc; ++i) {
        if (!qstrcmp(argv[i], "-no-gui"))
            nogui = true;        
        if (!qstrcmp(argv[i], "-qml"))
            forceQml = true;
        if (!qstrcmp(argv[i], "-miles"))
            miles = true;
        if (!qstrcmp(argv[i], "-no-console"))
            noConsole = true;
        if (!qstrcmp(argv[i], "-test-resistance"))
            testResistance = true;
        if (!qstrcmp(argv[i], "-no-log"))
            nologs = true;
        if (!qstrcmp(argv[i], "-no-write-resistance"))
            noWriteResistance = true;
        if (!qstrcmp(argv[i], "-no-heart-service"))
            noHeartService = true;        
        if (!qstrcmp(argv[i], "-heart-service"))
            noHeartService = false;
        if (!qstrcmp(argv[i], "-only-virtualbike"))
            onlyVirtualBike = true;
        if (!qstrcmp(argv[i], "-only-virtualtreadmill"))
            onlyVirtualTreadmill = true;
        if (!qstrcmp(argv[i], "-train"))
        {
            trainProgram = argv[++i];
        }
        if (!qstrcmp(argv[i], "-name"))
        {
            deviceName = argv[++i];
        }
        if (!qstrcmp(argv[i], "-poll-device-time"))
        {
            pollDeviceTime = atol(argv[++i]);
        }
        if (!qstrcmp(argv[i], "-bike-resistance-gain"))
        {
            bikeResistanceGain = atoi(argv[++i]);
        }
        if (!qstrcmp(argv[i], "-bike-resistance-offset"))
        {
            bikeResistanceOffset = atoi(argv[++i]);
        }
    }

    if(nogui)
        return new QCoreApplication(argc, argv);
    else if(forceQml)
        return new QGuiApplication(argc, argv);
    else        
    {
        QApplication* a = new QApplication(argc, argv);

        a->setStyle(QStyleFactory::create("Fusion"));

        /*QFont defaultFont = QApplication::font();
        defaultFont.setPointSize(defaultFont.pointSize()+2);
        qApp->setFont(defaultFont);*/

        // modify palette to dark
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window,QColor(53,53,53));
        darkPalette.setColor(QPalette::WindowText,Qt::white);
        darkPalette.setColor(QPalette::Disabled,QPalette::WindowText,QColor(127,127,127));
        darkPalette.setColor(QPalette::Base,QColor(42,42,42));
        darkPalette.setColor(QPalette::AlternateBase,QColor(66,66,66));
        darkPalette.setColor(QPalette::ToolTipBase,Qt::white);
        darkPalette.setColor(QPalette::ToolTipText,Qt::white);
        darkPalette.setColor(QPalette::Text,Qt::white);
        darkPalette.setColor(QPalette::Disabled,QPalette::Text,QColor(127,127,127));
        darkPalette.setColor(QPalette::Dark,QColor(35,35,35));
        darkPalette.setColor(QPalette::Shadow,QColor(20,20,20));
        darkPalette.setColor(QPalette::Button,QColor(53,53,53));
        darkPalette.setColor(QPalette::ButtonText,Qt::white);
        darkPalette.setColor(QPalette::Disabled,QPalette::ButtonText,QColor(127,127,127));
        darkPalette.setColor(QPalette::BrightText,Qt::red);
        darkPalette.setColor(QPalette::Link,QColor(42,130,218));
        darkPalette.setColor(QPalette::Highlight,QColor(42,130,218));
        darkPalette.setColor(QPalette::Disabled,QPalette::Highlight,QColor(80,80,80));
        darkPalette.setColor(QPalette::HighlightedText,Qt::white);
        darkPalette.setColor(QPalette::Disabled,QPalette::HighlightedText,QColor(127,127,127));

        qApp->setPalette(darkPalette);

        return a;
    }
}

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    QString txt;
    switch (type) {
    case QtInfoMsg:
        txt = QString("Info: %1 %2 %3\n").arg(file).arg(function).arg(msg);
        break;
    case QtDebugMsg:
        txt = QString("Debug: %1 %2 %3\n").arg(file).arg(function).arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1 %2 %3\n").arg(file).arg(function).arg(msg);
    break;
    case QtCriticalMsg:
        txt = QString("Critical: %1 %2 %3\n").arg(file).arg(function).arg(msg);
    break;
    case QtFatalMsg:
        txt = QString("Fatal: %1 %2 %3\n").arg(file).arg(function).arg(msg);
        abort();
    }

    if(nologs == false)
    {
        QString path = "";
#if defined(Q_OS_ANDROID) || defined(Q_OS_MACOS) || defined(Q_OS_OSX)
        path = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/";
#endif

        QFile outFile(path + logfilename);
        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream ts(&outFile);
        ts << txt;

        fprintf(stderr, txt.toLocal8Bit());
    }
}

int main(int argc, char *argv[])
{
#ifdef Q_OS_LINUX
#ifndef Q_OS_ANDROID
    if (getuid()) 
    {
        printf("Runme as root!\n");
        return -1;
    }
    else printf("%s", "OK, you are root.\n");
#endif
#endif

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    QScopedPointer<QCoreApplication> app(createApplication(argc, argv));
#else
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QScopedPointer<QGuiApplication> app(new QGuiApplication(argc, argv));
#endif

    app->setOrganizationName("Roberto Viola");
    app->setOrganizationDomain("robertoviola.cloud");
    app->setApplicationName("qDomyos-Zwift");

    QSettings settings;
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    bool defaultNoHeartService = !noHeartService;

    // Android 10 doesn't support multiple services for peripheral mode
    if(QOperatingSystemVersion::current() >= QOperatingSystemVersion(QOperatingSystemVersion::Android, 10))
        settings.setValue("bike_heartrate_service", true);

    noHeartService = settings.value("bike_heartrate_service", defaultNoHeartService).toBool();
    bikeResistanceOffset = settings.value("bike_resistance_offset", bikeResistanceOffset).toInt();
    bikeResistanceGain = settings.value("bike_resistance_gain", bikeResistanceGain).toInt();
#else
    settings.setValue("miles_unit", miles);
#endif

    qInstallMessageHandler(myMessageOutput);
    qDebug() << "version 1.4.7";

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if(!forceQml)
    {
        if(onlyVirtualBike)
        {
            virtualbike* V = new virtualbike(new bike(), noWriteResistance, noHeartService);
            Q_UNUSED(V)
            return app->exec();
        }
        else if(onlyVirtualTreadmill)
        {
            virtualtreadmill* V = new virtualtreadmill(new treadmill(), noHeartService);
            Q_UNUSED(V)
            return app->exec();
        }
    }
#endif
    bluetooth* bl = new bluetooth(!nologs, deviceName, noWriteResistance, noHeartService, pollDeviceTime, noConsole, testResistance, bikeResistanceOffset, bikeResistanceGain);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if(forceQml)
#endif
    {
        QQmlApplicationEngine engine;
        const QUrl url(QStringLiteral("qrc:/main.qml"));
        QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                         qobject_cast<QGuiApplication *>(app.data()), [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        }, Qt::QueuedConnection);

#if defined(Q_OS_ANDROID)
        auto  result = QtAndroid::checkPermission(QString("android.permission.WRITE_EXTERNAL_STORAGE"));
        if(result == QtAndroid::PermissionResult::Denied){
            QtAndroid::PermissionResultMap resultHash = QtAndroid::requestPermissionsSync(QStringList({"android.permission.WRITE_EXTERNAL_STORAGE"}));
            if(resultHash["android.permission.WRITE_EXTERNAL_STORAGE"] == QtAndroid::PermissionResult::Denied)
                qDebug() << "log unwritable!";
        }
        result = QtAndroid::checkPermission(QString("android.permission.READ_EXTERNAL_STORAGE"));
        if(result == QtAndroid::PermissionResult::Denied){
            QtAndroid::PermissionResultMap resultHash = QtAndroid::requestPermissionsSync(QStringList({"android.permission.READ_EXTERNAL_STORAGE"}));
            if(resultHash["android.permission.READ_EXTERNAL_STORAGE"] == QtAndroid::PermissionResult::Denied)
                qDebug() << "log unwritable!";
        }
#endif
        engine.load(url);
        new homeform(&engine, bl);

        {
#ifdef Q_OS_ANDROID
            KeepAwakeHelper helper;
#elif defined Q_OS_MACOS
            lockScreen();
#endif                        
            // screen and CPU will stay awake during this section
            // lock will be released when helper object goes out of scope
            return app->exec();
        }
#ifdef Q_OS_MACOS
        unlockScreen();
#endif
    }

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS)
    if (qobject_cast<QApplication *>(app.data())) {
        // start GUI version...
        MainWindow* W = 0;
        if(trainProgram.isEmpty())
            W = new MainWindow(bl);
        else
            W = new MainWindow(bl, trainProgram);
        W->show();
    } else {
        // start non-GUI version...
    }
    return app->exec();
#endif
}
