#pragma once

#include "core/model/scene_types.h"

#include <QString>
#include <QVector>

namespace demo {

struct ProcessingConfig {
    int targetFps = 24;
    int processingWidth = 960;
    int processingHeight = 540;
    int maxPaths = 6;
    double minAreaRatio = 0.0025;
    double cannyLowThreshold = 48.0;
    double cannyHighThreshold = 128.0;
    int gaussianKernelSize = 5;
    int morphologyIterations = 1;
    double approximationEpsilonRatio = 0.015;
    int resamplePoints = 48;
    double stabilityBlend = 0.7;
    double matchDistanceThreshold = 0.12;
    bool loopVideo = true;
};

struct NetworkConfig {
    QString sourceId = QStringLiteral("demo-source");
    quint16 listenPort = 50010;
    int fragmentTimeoutMs = 200;
    int maxPacketSize = 1200;
    QVector<DeviceTarget> targets;
};

struct AppConfig {
    QString name = QStringLiteral("DEMO");
    ProcessingConfig processing;
    NetworkConfig network;
    QVector<ZoneConfig> zones;

    const ZoneConfig *findZone(const QString &zoneId) const;
};

class AppConfigLoader {
public:
    static bool loadFromFile(const QString &path, AppConfig *config, QString *error);
};

}  // namespace demo
