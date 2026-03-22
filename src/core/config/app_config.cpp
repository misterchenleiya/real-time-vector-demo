#include "core/config/app_config.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace demo {

namespace {

QString requireString(const QJsonObject &object, const QString &key, const QString &fallback = {})
{
    const QJsonValue value = object.value(key);
    return value.isString() ? value.toString() : fallback;
}

double readDouble(const QJsonObject &object, const QString &key, double fallback)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toDouble() : fallback;
}

int readInt(const QJsonObject &object, const QString &key, int fallback)
{
    const QJsonValue value = object.value(key);
    return value.isDouble() ? value.toInt(fallback) : fallback;
}

bool readBool(const QJsonObject &object, const QString &key, bool fallback)
{
    const QJsonValue value = object.value(key);
    return value.isBool() ? value.toBool(fallback) : fallback;
}

}  // namespace

const ZoneConfig *AppConfig::findZone(const QString &zoneId) const
{
    for (const ZoneConfig &zone : zones) {
        if (zone.id == zoneId) {
            return &zone;
        }
    }
    return nullptr;
}

bool AppConfigLoader::loadFromFile(const QString &path, AppConfig *config, QString *error)
{
    if (config == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("The config output object is null.");
        }
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to open config file: %1").arg(file.errorString());
        }
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to parse config JSON: %1").arg(parseError.errorString());
        }
        return false;
    }

    const QJsonObject root = document.object();
    AppConfig loaded;
    loaded.name = requireString(root, QStringLiteral("name"), loaded.name);

    if (root.contains(QStringLiteral("processing")) && root.value(QStringLiteral("processing")).isObject()) {
        const QJsonObject processing = root.value(QStringLiteral("processing")).toObject();
        loaded.processing.targetFps = readInt(processing, QStringLiteral("targetFps"), loaded.processing.targetFps);
        loaded.processing.processingWidth = readInt(processing, QStringLiteral("processingWidth"), loaded.processing.processingWidth);
        loaded.processing.processingHeight = readInt(processing, QStringLiteral("processingHeight"), loaded.processing.processingHeight);
        loaded.processing.maxPaths = readInt(processing, QStringLiteral("maxPaths"), loaded.processing.maxPaths);
        loaded.processing.minAreaRatio = readDouble(processing, QStringLiteral("minAreaRatio"), loaded.processing.minAreaRatio);
        loaded.processing.cannyLowThreshold = readDouble(processing, QStringLiteral("cannyLowThreshold"), loaded.processing.cannyLowThreshold);
        loaded.processing.cannyHighThreshold = readDouble(processing, QStringLiteral("cannyHighThreshold"), loaded.processing.cannyHighThreshold);
        loaded.processing.gaussianKernelSize = readInt(processing, QStringLiteral("gaussianKernelSize"), loaded.processing.gaussianKernelSize);
        loaded.processing.morphologyIterations = readInt(processing, QStringLiteral("morphologyIterations"), loaded.processing.morphologyIterations);
        loaded.processing.approximationEpsilonRatio = readDouble(processing, QStringLiteral("approximationEpsilonRatio"), loaded.processing.approximationEpsilonRatio);
        loaded.processing.resamplePoints = readInt(processing, QStringLiteral("resamplePoints"), loaded.processing.resamplePoints);
        loaded.processing.stabilityBlend = readDouble(processing, QStringLiteral("stabilityBlend"), loaded.processing.stabilityBlend);
        loaded.processing.matchDistanceThreshold = readDouble(processing, QStringLiteral("matchDistanceThreshold"), loaded.processing.matchDistanceThreshold);
        loaded.processing.loopVideo = readBool(processing, QStringLiteral("loopVideo"), loaded.processing.loopVideo);
    }

    if (root.contains(QStringLiteral("network")) && root.value(QStringLiteral("network")).isObject()) {
        const QJsonObject network = root.value(QStringLiteral("network")).toObject();
        loaded.network.sourceId = requireString(network, QStringLiteral("sourceId"), loaded.network.sourceId);
        loaded.network.listenPort = static_cast<quint16>(readInt(network, QStringLiteral("listenPort"), loaded.network.listenPort));
        loaded.network.fragmentTimeoutMs = readInt(network, QStringLiteral("fragmentTimeoutMs"), loaded.network.fragmentTimeoutMs);
        loaded.network.maxPacketSize = readInt(network, QStringLiteral("maxPacketSize"), loaded.network.maxPacketSize);

        if (network.contains(QStringLiteral("targets")) && network.value(QStringLiteral("targets")).isArray()) {
            const QJsonArray targets = network.value(QStringLiteral("targets")).toArray();
            for (const QJsonValue &value : targets) {
                if (!value.isObject()) {
                    continue;
                }
                const QJsonObject targetObject = value.toObject();
                DeviceTarget target;
                target.id = requireString(targetObject, QStringLiteral("id"));
                target.name = requireString(targetObject, QStringLiteral("name"), target.id);
                target.host = requireString(targetObject, QStringLiteral("host"), target.host);
                target.port = static_cast<quint16>(readInt(targetObject, QStringLiteral("port"), 0));
                target.zoneId = requireString(targetObject, QStringLiteral("zoneId"), QStringLiteral("full"));
                target.scaleX = readDouble(targetObject, QStringLiteral("scaleX"), 1.0);
                target.scaleY = readDouble(targetObject, QStringLiteral("scaleY"), 1.0);
                target.offsetX = readDouble(targetObject, QStringLiteral("offsetX"), 0.0);
                target.offsetY = readDouble(targetObject, QStringLiteral("offsetY"), 0.0);
                if (!target.id.isEmpty() && target.port > 0) {
                    loaded.network.targets.push_back(target);
                }
            }
        }
    }

    if (root.contains(QStringLiteral("zones")) && root.value(QStringLiteral("zones")).isArray()) {
        const QJsonArray zones = root.value(QStringLiteral("zones")).toArray();
        for (const QJsonValue &value : zones) {
            if (!value.isObject()) {
                continue;
            }
            const QJsonObject zoneObject = value.toObject();
            ZoneConfig zone;
            zone.id = requireString(zoneObject, QStringLiteral("id"));
            zone.rect = QRectF(
                readDouble(zoneObject, QStringLiteral("x"), 0.0),
                readDouble(zoneObject, QStringLiteral("y"), 0.0),
                readDouble(zoneObject, QStringLiteral("width"), 1.0),
                readDouble(zoneObject, QStringLiteral("height"), 1.0));
            if (!zone.id.isEmpty()) {
                loaded.zones.push_back(zone);
            }
        }
    }

    if (loaded.zones.isEmpty()) {
        loaded.zones.push_back({QStringLiteral("full"), QRectF(0.0, 0.0, 1.0, 1.0)});
    }

    *config = loaded;
    return true;
}

}  // namespace demo
