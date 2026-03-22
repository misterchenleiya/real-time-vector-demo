#pragma once

#include <QPointF>
#include <QRectF>
#include <QVector>

#include <cmath>

namespace demo {

inline QRectF normalizedSceneRect()
{
    return QRectF(0.0, 0.0, 1.0, 1.0);
}

inline QRectF boundingRect(const QVector<QPointF> &points)
{
    if (points.isEmpty()) {
        return {};
    }

    qreal minX = points.front().x();
    qreal minY = points.front().y();
    qreal maxX = minX;
    qreal maxY = minY;

    for (const QPointF &point : points) {
        minX = std::min(minX, point.x());
        minY = std::min(minY, point.y());
        maxX = std::max(maxX, point.x());
        maxY = std::max(maxY, point.y());
    }

    return QRectF(QPointF(minX, minY), QPointF(maxX, maxY)).normalized();
}

inline double squaredDistance(const QPointF &left, const QPointF &right)
{
    const double dx = left.x() - right.x();
    const double dy = left.y() - right.y();
    return (dx * dx) + (dy * dy);
}

}  // namespace demo
