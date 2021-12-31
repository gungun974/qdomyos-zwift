#include "virtualrower.h"
#include "ftmsrower.h"

#include <QDataStream>
#include <QMetaEnum>
#include <QSettings>
#include <QtMath>
#include <chrono>

using namespace std::chrono_literals;

virtualrower::virtualrower(bluetoothdevice *t, bool noWriteResistance, bool noHeartService) {
    Rower = t;

    this->noHeartService = noHeartService;

    QSettings settings;
    bool heart_only = settings.value(QStringLiteral("virtual_device_onlyheart"), false).toBool();

    Q_UNUSED(noWriteResistance)

    {
        //! [Advertising Data]
        advertisingData.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);
        advertisingData.setIncludePowerLevel(true);
        advertisingData.setLocalName(QStringLiteral("PM5"));
        QList<QBluetoothUuid> services;

        if (!heart_only) {
            services << ((QBluetoothUuid::ServiceClassUuid)0x1826);
        }
        if (!this->noHeartService || heart_only) {
            services << QBluetoothUuid::HeartRate;
        }

        services << ((QBluetoothUuid::ServiceClassUuid)0xFF00);

        advertisingData.setServices(services);
        //! [Advertising Data]

        if (!heart_only) {

            serviceDataFIT.setType(QLowEnergyServiceData::ServiceTypePrimary);
            QLowEnergyCharacteristicData charDataFIT;
            charDataFIT.setUuid((QBluetoothUuid::CharacteristicType)0x2ACC); // FitnessMachineFeatureCharacteristicUuid
            QByteArray valueFIT;
            valueFIT.append((char)0xA2); // cadence, pace and resistance level supported
            valueFIT.append((char)0x45); // stride count,heart rate, power supported
            valueFIT.append((char)0x00);
            valueFIT.append((char)0x00);
            valueFIT.append((char)0x0C); // resistance and power target supported
            valueFIT.append((char)0x00);
            valueFIT.append((char)0x00);
            valueFIT.append((char)0x00);
            charDataFIT.setValue(valueFIT);
            charDataFIT.setProperties(QLowEnergyCharacteristic::Read);

            QLowEnergyCharacteristicData charDataFIT2;
            charDataFIT2.setUuid(
                (QBluetoothUuid::CharacteristicType)0x2AD6); // supported_resistance_level_rangeCharacteristicUuid
            charDataFIT2.setProperties(QLowEnergyCharacteristic::Read);
            QByteArray valueFIT2;
            valueFIT2.append((char)0x0A); // min resistance value
            valueFIT2.append((char)0x00); // min resistance value
            valueFIT2.append((char)0x96); // max resistance value
            valueFIT2.append((char)0x00); // max resistance value
            valueFIT2.append((char)0x0A); // step resistance
            valueFIT2.append((char)0x00); // step resistance
            charDataFIT2.setValue(valueFIT2);

            QLowEnergyCharacteristicData charDataFIT3;
            charDataFIT3.setUuid((QBluetoothUuid::CharacteristicType)0x2AD9); // Fitness Machine Control Point
            charDataFIT3.setProperties(QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::Indicate);
            const QLowEnergyDescriptorData cpClientConfig(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                          QByteArray(2, 0));
            charDataFIT3.addDescriptor(cpClientConfig);

            QLowEnergyCharacteristicData charDataFIT4;
            charDataFIT4.setUuid((QBluetoothUuid::CharacteristicType)0x2AD1); // rower
            charDataFIT4.setProperties(QLowEnergyCharacteristic::Notify | QLowEnergyCharacteristic::Read);
            QByteArray descriptor;
            descriptor.append((char)0x01);
            descriptor.append((char)0x00);
            const QLowEnergyDescriptorData clientConfig4(QBluetoothUuid::ClientCharacteristicConfiguration, descriptor);
            charDataFIT4.addDescriptor(clientConfig4);

            QLowEnergyCharacteristicData charDataFIT5;
            charDataFIT5.setUuid((QBluetoothUuid::CharacteristicType)0x2ADA); // Fitness Machine status
            charDataFIT5.setProperties(QLowEnergyCharacteristic::Notify);
            QByteArray descriptor5;
            descriptor5.append((char)0x01);
            descriptor5.append((char)0x00);
            const QLowEnergyDescriptorData clientConfig5(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                         descriptor5);
            charDataFIT5.addDescriptor(clientConfig5);

            QLowEnergyCharacteristicData charDataFIT6;
            charDataFIT6.setUuid((QBluetoothUuid::CharacteristicType)0x2AD3);
            charDataFIT6.setProperties(QLowEnergyCharacteristic::Notify | QLowEnergyCharacteristic::Read);
            QByteArray valueFIT6;
            valueFIT6.append((char)0x00);
            valueFIT6.append((char)0x01);
            charDataFIT6.setValue(valueFIT6);
            QByteArray descriptor6;
            descriptor6.append((char)0x01);
            descriptor6.append((char)0x00);
            const QLowEnergyDescriptorData clientConfig6(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                         descriptor6);
            charDataFIT6.addDescriptor(clientConfig6);
            charDataFIT6.setProperties(QLowEnergyCharacteristic::Read);

            serviceDataFIT.setUuid((QBluetoothUuid::ServiceClassUuid)0x1826); // FitnessMachineServiceUuid
            serviceDataFIT.addCharacteristic(charDataFIT);
            serviceDataFIT.addCharacteristic(charDataFIT2);
            serviceDataFIT.addCharacteristic(charDataFIT3);
            serviceDataFIT.addCharacteristic(charDataFIT4);
            serviceDataFIT.addCharacteristic(charDataFIT5);
            serviceDataFIT.addCharacteristic(charDataFIT6);
        }

        if (!this->noHeartService || heart_only) {

            QLowEnergyCharacteristicData charDataHR;
            charDataHR.setUuid(QBluetoothUuid::HeartRateMeasurement);
            charDataHR.setValue(QByteArray(2, 0));
            charDataHR.setProperties(QLowEnergyCharacteristic::Notify);
            const QLowEnergyDescriptorData clientConfigHR(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                          QByteArray(2, 0));
            charDataHR.addDescriptor(clientConfigHR);

            serviceDataHR.setType(QLowEnergyServiceData::ServiceTypePrimary);
            serviceDataHR.setUuid(QBluetoothUuid::HeartRate);
            serviceDataHR.addCharacteristic(charDataHR);
        }

        //! [Start Advertising]
        leController = QLowEnergyController::createPeripheral();
        Q_ASSERT(leController);

        serviceFIT = leController->addService(serviceDataFIT);

        if (!this->noHeartService || heart_only) {
            serviceHR = leController->addService(serviceDataHR);
        }

        QObject::connect(serviceFIT, &QLowEnergyService::characteristicChanged, this,
                         &virtualrower::characteristicChanged);

        bool bluetooth_relaxed = settings.value(QStringLiteral("bluetooth_relaxed"), false).toBool();
        QLowEnergyAdvertisingParameters pars = QLowEnergyAdvertisingParameters();
        if (!bluetooth_relaxed) {
            pars.setInterval(100, 100);
        }

        leController->startAdvertising(pars, advertisingData, advertisingData);

        //! [Start Advertising]
    }

    //! [Provide Heartbeat]
    QObject::connect(&rowerTimer, &QTimer::timeout, this, &virtualrower::rowerProvider);
    rowerTimer.start(1s);
    //! [Provide Heartbeat]
    QObject::connect(leController, &QLowEnergyController::disconnected, this, &virtualrower::reconnect);
    QObject::connect(
        leController,
        static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error), this,
        &virtualrower::error);
}

void virtualrower::characteristicChanged(const QLowEnergyCharacteristic &characteristic, const QByteArray &newValue) {
    QByteArray reply;
    QSettings settings;
    bool force_resistance = settings.value(QStringLiteral("virtualrower_forceresistance"), true).toBool();
    bool erg_mode = settings.value(QStringLiteral("zwift_erg"), false).toBool();
    //    double erg_filter_upper =
    //        settings.value(QStringLiteral("zwift_erg_filter"), 0.0).toDouble(); //
    //        NOTE:clang-analyzer-deadcode.DeadStores
    //    double erg_filter_lower = settings.value(QStringLiteral("zwift_erg_filter_down"), 0.0)
    //                                  .toDouble(); // NOTE:clang-analyzer-deadcode.DeadStores
    qDebug() << QStringLiteral("characteristicChanged ") + QString::number(characteristic.uuid().toUInt16()) +
                    QStringLiteral(" ") + newValue.toHex(' ');

    lastFTMSFrameReceived = QDateTime::currentMSecsSinceEpoch();

    switch (characteristic.uuid().toUInt16()) {
        // TODO
        /*

    case 0x2AD9: // Fitness Machine Control Point

        if ((char)newValue.at(0) == FTMS_SET_TARGET_RESISTANCE_LEVEL) {

            // Set Target Resistance
            uint8_t uresistance = newValue.at(1);
            uresistance = uresistance / 10;
            if (force_resistance && !erg_mode) {
                rower->changeResistance(uresistance);
            }
            qDebug() << QStringLiteral("new requested resistance ") + QString::number(uresistance) +
                            QStringLiteral(" enabled ") + force_resistance;
            reply.append((quint8)FTMS_RESPONSE_CODE);
            reply.append((quint8)FTMS_SET_TARGET_RESISTANCE_LEVEL);
            reply.append((quint8)FTMS_SUCCESS);
        } else if ((char)newValue.at(0) == FTMS_SET_INDOOR_rower_SIMULATION_PARAMS) // simulation parameter

        {
            qDebug() << QStringLiteral("indoor rower simulation parameters");
            reply.append((quint8)FTMS_RESPONSE_CODE);
            reply.append((quint8)FTMS_SET_INDOOR_rower_SIMULATION_PARAMS);
            reply.append((quint8)FTMS_SUCCESS);

            int16_t iresistance = (((uint8_t)newValue.at(3)) + (newValue.at(4) << 8));
            slopeChanged(iresistance);
        } else if ((char)newValue.at(0) == FTMS_SET_TARGET_POWER) // erg mode

        {
            qDebug() << QStringLiteral("erg mode");
            reply.append((quint8)FTMS_RESPONSE_CODE);
            reply.append((quint8)FTMS_SET_TARGET_POWER);
            reply.append((quint8)FTMS_SUCCESS);

            uint16_t power = (((uint8_t)newValue.at(1)) + (newValue.at(2) << 8));
            powerChanged(power);
        } else if ((char)newValue.at(0) == FTMS_START_RESUME) {
            qDebug() << QStringLiteral("start simulation!");

            reply.append((quint8)FTMS_RESPONSE_CODE);
            reply.append((quint8)FTMS_START_RESUME);
            reply.append((quint8)FTMS_SUCCESS);
        } else if ((char)newValue.at(0) == FTMS_REQUEST_CONTROL) {
            qDebug() << QStringLiteral("control requested");

            reply.append((quint8)FTMS_RESPONSE_CODE);
            reply.append((char)FTMS_REQUEST_CONTROL);
            reply.append((quint8)FTMS_SUCCESS);
        } else {
            qDebug() << QStringLiteral("not supported");

            reply.append((quint8)FTMS_RESPONSE_CODE);
            reply.append((quint8)newValue.at(0));
            reply.append((quint8)FTMS_NOT_SUPPORTED);
        }

        QLowEnergyCharacteristic characteristic =
            serviceFIT->characteristic((QBluetoothUuid::CharacteristicType)0x2AD9);
        Q_ASSERT(characteristic.isValid());
        if (leController->state() != QLowEnergyController::ConnectedState) {
            qDebug() << QStringLiteral("virtual rower not connected");

            return;
        }
        writeCharacteristic(serviceFIT, characteristic, reply);
        break;
        */
    }
}

void virtualrower::writeCharacteristic(QLowEnergyService *service, const QLowEnergyCharacteristic &characteristic,
                                       const QByteArray &value) {
    try {
        qDebug() << QStringLiteral("virtualrower::writeCharacteristic ") + service->serviceName() +
                        QStringLiteral(" ") + characteristic.name() + QStringLiteral(" ") + value.toHex(' ');
        service->writeCharacteristic(characteristic, value); // Potentially causes notification.
    } catch (...) {
        qDebug() << QStringLiteral("virtual rower error!");
    }
}

void virtualrower::reconnect() {

    QSettings settings;
    bool bluetooth_relaxed = settings.value(QStringLiteral("bluetooth_relaxed"), false).toBool();

    if (bluetooth_relaxed) {
        return;
    }

    bool heart_only = settings.value(QStringLiteral("virtual_device_onlyheart"), false).toBool();

    qDebug() << QStringLiteral("virtualrower::reconnect");
    leController->disconnectFromDevice();

#ifndef Q_OS_IOS
    serviceFIT = leController->addService(serviceDataFIT);
    if (!this->noHeartService || heart_only)
        serviceHR = leController->addService(serviceDataHR);
#endif

    QLowEnergyAdvertisingParameters pars;
    pars.setInterval(100, 100);
    leController->startAdvertising(pars, advertisingData, advertisingData);
}

void virtualrower::rowerProvider() {

    QSettings settings;
    bool heart_only = settings.value(QStringLiteral("virtual_device_onlyheart"), false).toBool();

    if (leController->state() != QLowEnergyController::ConnectedState) {
        qDebug() << QStringLiteral("virtual rower not connected");

        return;
    } else {
        bool bluetooth_relaxed = settings.value(QStringLiteral("bluetooth_relaxed"), false).toBool();
        bool bluetooth_30m_hangs = settings.value(QStringLiteral("bluetooth_30m_hangs"), false).toBool();
        if (bluetooth_relaxed) {

            leController->stopAdvertising();
        }

        if (lastFTMSFrameReceived > 0 && QDateTime::currentMSecsSinceEpoch() > (lastFTMSFrameReceived + 5000) &&
            bluetooth_30m_hangs) {
            lastFTMSFrameReceived = 0;
            qDebug() << QStringLiteral("virtual rower timeout, reconnecting...");

            reconnect();
            return;
        }

        qDebug() << QStringLiteral("virtual rower connected");
    }

    QByteArray value;

    if (!heart_only) {

        value.append((char)0xA0); // resistance level, power and speed
        value.append((char)0x02); // heart rate

        value.append((char)((uint8_t)Rower->currentCadence().value() & 0xFF)); // Stroke Rate

        value.append((char)((uint16_t)(((rower *)Rower)->currentStrokesCount().value()) & 0xFF));        // Stroke Count
        value.append((char)(((uint16_t)(((rower *)Rower)->currentStrokesCount().value()) >> 8) & 0xFF)); // Stroke Count

        value.append((char)(((uint16_t)Rower->wattsMetric().value()) & 0xFF));      // watts
        value.append((char)(((uint16_t)Rower->wattsMetric().value()) >> 8) & 0xFF); // watts

        value.append((char)((uint16_t)(Rower->currentResistance().value()) & 0xFF));        // resistance
        value.append((char)(((uint16_t)(Rower->currentResistance().value()) >> 8) & 0xFF)); // resistance

        value.append(char(Rower->currentHeart().value())); // Actual value.
        value.append((char)0);                             // Bkool FTMS protocol HRM offset 1280 fix

        if (!serviceFIT) {
            qDebug() << QStringLiteral("serviceFIT not available");

            return;
        }

        QLowEnergyCharacteristic characteristic =
            serviceFIT->characteristic((QBluetoothUuid::CharacteristicType)0x2AD2);
        Q_ASSERT(characteristic.isValid());
        if (leController->state() != QLowEnergyController::ConnectedState) {
            qDebug() << QStringLiteral("virtual rower not connected");

            return;
        }
        writeCharacteristic(serviceFIT, characteristic, value);
    }
    // characteristic
    //        = service->characteristic((QBluetoothUuid::CharacteristicType)0x2AD9); // Fitness Machine Control Point
    // Q_ASSERT(characteristic.isValid());
    // service->readCharacteristic(characteristic);

    if (!this->noHeartService || heart_only) {
        if (!serviceHR) {
            qDebug() << QStringLiteral("serviceHR not available");

            return;
        }

        QByteArray valueHR;
        valueHR.append(char(0));                                   // Flags that specify the format of the value.
        valueHR.append(char(Rower->metrics_override_heartrate())); // Actual value.
        QLowEnergyCharacteristic characteristicHR = serviceHR->characteristic(QBluetoothUuid::HeartRateMeasurement);

        Q_ASSERT(characteristicHR.isValid());
        if (leController->state() != QLowEnergyController::ConnectedState) {
            qDebug() << QStringLiteral("virtual rower not connected");

            return;
        }
        writeCharacteristic(serviceHR, characteristicHR, valueHR);
    }
}

bool virtualrower::connected() {
    if (!leController) {

        return false;
    }
    return leController->state() == QLowEnergyController::ConnectedState;
}

void virtualrower::error(QLowEnergyController::Error newError) {

    QMetaEnum metaEnum = QMetaEnum::fromType<QLowEnergyController::Error>();
    qDebug() << QStringLiteral("virtualrower::controller:ERROR ") +
                    QString::fromLocal8Bit(metaEnum.valueToKey(newError));

    if (newError != QLowEnergyController::RemoteHostClosedError) {
        reconnect();
    }
}