#include "core/config/app_config.h"
#include "core/network/udp_device_receiver.h"
#include "core/processing/sender_engine.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>
#include <QObject>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("demo_cli"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("real-time-vector-demo CLI"));
    parser.addHelpOption();

    QCommandLineOption modeOption(
        QStringList() << QStringLiteral("m") << QStringLiteral("mode"),
        QStringLiteral("Run mode: sender or receiver"),
        QStringLiteral("mode"),
        QStringLiteral("receiver"));
    QCommandLineOption configOption(
        QStringList() << QStringLiteral("c") << QStringLiteral("config"),
        QStringLiteral("Config file path"),
        QStringLiteral("path"));
    QCommandLineOption mediaOption(
        QStringList() << QStringLiteral("media"),
        QStringLiteral("Media file path used in sender mode"),
        QStringLiteral("path"));

    parser.addOption(modeOption);
    parser.addOption(configOption);
    parser.addOption(mediaOption);
    parser.process(app);

    if (!parser.isSet(configOption)) {
        qCritical() << "--config is required";
        return 1;
    }

    demo::AppConfig config;
    QString error;
    if (!demo::AppConfigLoader::loadFromFile(parser.value(configOption), &config, &error)) {
        qCritical().noquote() << error;
        return 1;
    }

    const QString mode = parser.value(modeOption).trimmed().toLower();
    if (mode == QStringLiteral("sender")) {
        if (!parser.isSet(mediaOption)) {
            qCritical() << "--media is required in sender mode";
            return 1;
        }

        demo::SenderEngine sender;
        QObject::connect(&sender, &demo::SenderEngine::errorOccurred, [](const QString &message) {
            qCritical().noquote() << message;
        });
        QObject::connect(&sender, &demo::SenderEngine::stateMessage, [](const QString &message) {
            qInfo().noquote() << message;
        });
        QObject::connect(&sender, &demo::SenderEngine::metricsUpdated, [](const demo::EngineMetrics &metrics) {
            qInfo().noquote()
                << QStringLiteral("processing=%1 send=%2 paths=%3 devices=%4 latency=%5ms")
                       .arg(metrics.processingFps, 0, 'f', 1)
                       .arg(metrics.sendFps, 0, 'f', 1)
                       .arg(metrics.lastPathCount)
                       .arg(metrics.lastDeviceCount)
                       .arg(metrics.lastFrameLatencyMs);
        });

        sender.configure(config, &error);
        if (!sender.loadMedia(parser.value(mediaOption), &error)) {
            qCritical().noquote() << error;
            return 1;
        }
        sender.start();
        return app.exec();
    }

    demo::UdpDeviceReceiver receiver;
    receiver.setFragmentTimeoutMs(config.network.fragmentTimeoutMs);
    QObject::connect(&receiver, &demo::UdpDeviceReceiver::receiverError, [](const QString &message) {
        qWarning().noquote() << message;
    });
    QObject::connect(&receiver, &demo::UdpDeviceReceiver::frameReceived, [](const demo::DeviceFrame &frame) {
        qInfo().noquote() << demo::describeFrame(frame);
    });

    if (!receiver.listen(config.network.listenPort, &error)) {
        qCritical().noquote() << error;
        return 1;
    }

    qInfo().noquote() << QStringLiteral("Receiver mode started. Listening on 127.0.0.1:%1").arg(config.network.listenPort);
    return app.exec();
}
