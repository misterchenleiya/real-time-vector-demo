#include "core/model/scene_types.h"

#include <QDebug>

namespace demo {

QPointF PathStroke::centroid() const
{
    if (points.isEmpty()) {
        return {};
    }

    qreal sumX = 0.0;
    qreal sumY = 0.0;
    for (const QPointF &point : points) {
        sumX += point.x();
        sumY += point.y();
    }
    const qreal count = static_cast<qreal>(points.size());
    return QPointF(sumX / count, sumY / count);
}

bool SceneFrame::isValid() const
{
    return !sourceId.isEmpty() && frameId > 0 && sceneSize.isValid();
}

bool DeviceFrame::isValid() const
{
    return !sourceId.isEmpty() && !deviceId.isEmpty() && frameId > 0;
}

qint64 currentTimestampMicros()
{
    return QDateTime::currentMSecsSinceEpoch() * 1000;
}

QString describeFrame(const DeviceFrame &frame)
{
    if (frame.clearRequested) {
        return QStringLiteral("%1/%2 clear").arg(frame.sourceId, frame.deviceId);
    }

    return QStringLiteral("%1/%2 frame=%3 paths=%4")
        .arg(frame.sourceId, frame.deviceId)
        .arg(frame.frameId)
        .arg(frame.paths.size());
}

QDataStream &operator<<(QDataStream &stream, const PathStroke &path)
{
    stream << path.id << path.color << path.closed << path.points;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, PathStroke &path)
{
    stream >> path.id >> path.color >> path.closed >> path.points;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const DeviceFrame &frame)
{
    stream << frame.sourceId
           << frame.deviceId
           << frame.deviceName
           << frame.zoneId
           << frame.frameId
           << frame.timestampUs
           << frame.sourceSize
           << frame.clearRequested
           << frame.paths;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, DeviceFrame &frame)
{
    stream >> frame.sourceId
           >> frame.deviceId
           >> frame.deviceName
           >> frame.zoneId
           >> frame.frameId
           >> frame.timestampUs
           >> frame.sourceSize
           >> frame.clearRequested
           >> frame.paths;
    return stream;
}

}  // namespace demo
