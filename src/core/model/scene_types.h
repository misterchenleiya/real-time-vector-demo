#pragma once

#include <QColor>
#include <QDataStream>
#include <QDateTime>
#include <QMetaType>
#include <QPointF>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QVector>

namespace demo {

struct PathStroke {
    QString id;
    QColor color = Qt::white;
    bool closed = true;
    QVector<QPointF> points;

    QPointF centroid() const;
};

struct SceneFrame {
    QString sourceId;
    quint64 frameId = 0;
    qint64 timestampUs = 0;
    QSize sceneSize;
    QVector<PathStroke> paths;

    bool isValid() const;
};

struct ZoneConfig {
    QString id;
    QRectF rect = QRectF(0.0, 0.0, 1.0, 1.0);
};

struct DeviceTarget {
    QString id;
    QString name;
    QString host = QStringLiteral("127.0.0.1");
    quint16 port = 0;
    QString zoneId;
    qreal scaleX = 1.0;
    qreal scaleY = 1.0;
    qreal offsetX = 0.0;
    qreal offsetY = 0.0;
};

struct DeviceFrame {
    QString sourceId;
    QString deviceId;
    QString deviceName;
    QString zoneId;
    quint64 frameId = 0;
    qint64 timestampUs = 0;
    QSize sourceSize;
    QVector<PathStroke> paths;

    bool isValid() const;
};

struct EngineMetrics {
    double processingFps = 0.0;
    double sendFps = 0.0;
    quint64 framesProcessed = 0;
    quint64 framesSent = 0;
    int lastPathCount = 0;
    int lastDeviceCount = 0;
    qint64 lastFrameLatencyMs = 0;
};

qint64 currentTimestampMicros();
QString describeFrame(const DeviceFrame &frame);

QDataStream &operator<<(QDataStream &stream, const PathStroke &path);
QDataStream &operator>>(QDataStream &stream, PathStroke &path);
QDataStream &operator<<(QDataStream &stream, const DeviceFrame &frame);
QDataStream &operator>>(QDataStream &stream, DeviceFrame &frame);

}  // namespace demo

Q_DECLARE_METATYPE(demo::SceneFrame)
Q_DECLARE_METATYPE(demo::DeviceFrame)
Q_DECLARE_METATYPE(demo::EngineMetrics)
