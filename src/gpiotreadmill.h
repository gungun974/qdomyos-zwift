#ifndef GPIOTREADMILL_H
#define GPIOTREADMILL_H

#include <QBluetoothDeviceDiscoveryAgent>
#include <QtBluetooth/qlowenergyadvertisingdata.h>
#include <QtBluetooth/qlowenergyadvertisingparameters.h>
#include <QtBluetooth/qlowenergycharacteristic.h>
#include <QtBluetooth/qlowenergycharacteristicdata.h>

#include <QtBluetooth/qlowenergycontroller.h>
#include <QtBluetooth/qlowenergydescriptordata.h>
#include <QtBluetooth/qlowenergyservice.h>
#include <QtBluetooth/qlowenergyservicedata.h>

#include <QtCore/qbytearray.h>

#ifndef Q_OS_ANDROID
#include <QtCore/qcoreapplication.h>
#else
#include <QtGui/qguiapplication.h>
#endif
#include <QtCore/qlist.h>
#include <QtCore/qmutex.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qtimer.h>

#include <QDateTime>
#include <QObject>

#include "treadmill.h"
#include "virtualbike.h"
#include "virtualtreadmill.h"

#ifdef Q_OS_IOS
#include "ios/lockscreen.h"
#endif

class gpiotreadmill : public treadmill {

    Q_OBJECT
  public:
    gpiotreadmill(uint32_t poolDeviceTime = 200, bool noConsole = false, bool noHeartService = false,
                  double forceInitSpeed = 0.0, double forceInitInclination = 0.0);
    bool connected();
    bool changeFanSpeed(uint8_t speed);

    void *VirtualTreadMill();
    void *VirtualDevice();

  private:
    bool noConsole = false;
    bool noHeartService = false;
    uint32_t pollDeviceTime = 200;
    bool searchStopped = false;
    uint8_t sec1Update = 0;
    uint8_t firstInit = 0;
    QDateTime lastTimeCharacteristicChanged;
    bool firstCharacteristicChanged = true;

    QTimer *refresh;
    virtualtreadmill *virtualTreadMill = nullptr;
    virtualbike *virtualBike = 0;

    bool initDone = false;
    bool initRequest = false;

    const uint8_t OUTPUT_SPEED_UP = 6;
    const uint8_t OUTPUT_SPEED_DOWN = 26;
    const uint8_t OUTPUT_INCLINE_UP = 4;
    const uint8_t OUTPUT_INCLINE_DOWN = 22;
    const uint8_t OUTPUT_START = 23;
    const uint8_t OUTPUT_STOP = 25;

    const uint16_t GPIO_KEEP_MS = 200;
    const uint16_t GPIO_REBOUND_MS = 200;

    void forceSpeed(double requestSpeed);
    void forceIncline(double requestIncline);

#ifdef Q_OS_IOS
    lockscreen *h = 0;
#endif

  Q_SIGNALS:
    void disconnected();
    void debug(QString string);
    void speedChanged(double speed);
    void packetReceived();

  public slots:
    void searchingStop();

  private slots:

    void changeInclinationRequested(double grade, double percentage);

    void update();
};

#endif // GPIOTREADMILL_H