#include "core/routing/frame_router.h"

#include "core/model/geometry.h"

#include <algorithm>

namespace demo {

namespace {

QVector<QPointF> mapPathToTarget(const QVector<QPointF> &points, const QRectF &zoneRect, const DeviceTarget &target)
{
    QVector<QPointF> mapped;
    mapped.reserve(points.size());
    for (const QPointF &point : points) {
        if (!zoneRect.contains(point)) {
            continue;
        }

        const qreal zoneX = (point.x() - zoneRect.x()) / zoneRect.width();
        const qreal zoneY = (point.y() - zoneRect.y()) / zoneRect.height();
        const qreal transformedX = (zoneX * target.scaleX) + target.offsetX;
        const qreal transformedY = (zoneY * target.scaleY) + target.offsetY;
        mapped.push_back(QPointF(
            std::clamp(transformedX, 0.0, 1.0),
            std::clamp(transformedY, 0.0, 1.0)));
    }
    return mapped;
}

}  // namespace

QVector<DeviceFrame> FrameRouter::route(const SceneFrame &sceneFrame, const AppConfig &config) const
{
    QVector<DeviceFrame> frames;
    if (!sceneFrame.isValid()) {
        return frames;
    }

    for (const DeviceTarget &target : config.network.targets) {
        const ZoneConfig *zone = config.findZone(target.zoneId);
        const QRectF zoneRect = zone != nullptr ? zone->rect : QRectF(0.0, 0.0, 1.0, 1.0);

        DeviceFrame deviceFrame;
        deviceFrame.sourceId = sceneFrame.sourceId;
        deviceFrame.deviceId = target.id;
        deviceFrame.deviceName = target.name;
        deviceFrame.zoneId = target.zoneId;
        deviceFrame.frameId = sceneFrame.frameId;
        deviceFrame.timestampUs = sceneFrame.timestampUs;
        deviceFrame.sourceSize = sceneFrame.sceneSize;

        for (const PathStroke &path : sceneFrame.paths) {
            const QRectF pathBounds = boundingRect(path.points);
            if (!zoneRect.intersects(pathBounds) && !zoneRect.contains(path.centroid())) {
                continue;
            }

            PathStroke mappedPath = path;
            mappedPath.points = mapPathToTarget(path.points, zoneRect, target);
            if (mappedPath.points.size() >= 2) {
                deviceFrame.paths.push_back(mappedPath);
            }
        }

        frames.push_back(deviceFrame);
    }

    return frames;
}

}  // namespace demo
