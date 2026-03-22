#include "app/gui/preview_widget.h"

#include <QPainterPath>
#include <QPainter>

#include <algorithm>

PreviewWidget::PreviewWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(640, 360);
    setAutoFillBackground(false);
}

void PreviewWidget::setSourceImage(const QImage &image)
{
    sourceImage_ = image;
    update();
}

void PreviewWidget::setSceneFrame(const demo::SceneFrame &frame)
{
    sceneFrame_ = frame;
    update();
}

void PreviewWidget::setDeviceFrame(const demo::DeviceFrame &frame)
{
    deviceFrame_ = frame;
    update();
}

void PreviewWidget::clearPreview()
{
    sourceImage_ = {};
    sceneFrame_ = {};
    deviceFrame_ = {};
    update();
}

QSize PreviewWidget::minimumSizeHint() const
{
    return QSize(640, 360);
}

QRectF PreviewWidget::contentRect() const
{
    const QRectF bounds = rect().adjusted(12, 12, -12, -12);
    if (sourceImage_.isNull()) {
        const qreal edge = std::min(bounds.width(), bounds.height());
        const QPointF origin(bounds.center().x() - edge / 2.0, bounds.center().y() - edge / 2.0);
        return QRectF(origin, QSizeF(edge, edge));
    }

    QSizeF size = sourceImage_.size();
    size.scale(bounds.size(), Qt::KeepAspectRatio);
    const QPointF origin(bounds.center().x() - size.width() / 2.0, bounds.center().y() - size.height() / 2.0);
    return QRectF(origin, size);
}

void PreviewWidget::drawPaths(QPainter *painter, const QRectF &rect) const
{
    const QVector<demo::PathStroke> *paths = nullptr;
    if (!deviceFrame_.paths.isEmpty()) {
        paths = &deviceFrame_.paths;
    } else if (!sceneFrame_.paths.isEmpty()) {
        paths = &sceneFrame_.paths;
    }

    if (paths == nullptr) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const demo::PathStroke &path : *paths) {
        if (path.points.size() < 2) {
            continue;
        }

        QPainterPath painterPath;
        const QPointF first(
            rect.left() + path.points.front().x() * rect.width(),
            rect.top() + path.points.front().y() * rect.height());
        painterPath.moveTo(first);
        for (int index = 1; index < path.points.size(); ++index) {
            const QPointF mapped(
                rect.left() + path.points[index].x() * rect.width(),
                rect.top() + path.points[index].y() * rect.height());
            painterPath.lineTo(mapped);
        }
        if (path.closed) {
            painterPath.closeSubpath();
        }

        painter->setPen(QPen(path.color, 2.0));
        painter->drawPath(painterPath);
    }
    painter->restore();
}

void PreviewWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.fillRect(rect(), QColor(18, 20, 24));

    const QRectF drawRect = contentRect();
    painter.fillRect(drawRect, QColor(10, 10, 12));

    if (!sourceImage_.isNull()) {
        painter.drawImage(drawRect, sourceImage_);
    } else {
        painter.setPen(QPen(QColor(70, 74, 82), 1.0, Qt::DashLine));
        painter.drawRect(drawRect);
    }

    drawPaths(&painter, drawRect);

    painter.setPen(QColor(230, 230, 235));
    if (!deviceFrame_.paths.isEmpty()) {
        painter.drawText(rect().adjusted(16, 16, -16, -16),
                         Qt::AlignTop | Qt::AlignLeft,
                         QStringLiteral("Remote %1 / %2").arg(deviceFrame_.sourceId, deviceFrame_.deviceId));
    } else if (!sceneFrame_.paths.isEmpty()) {
        painter.drawText(rect().adjusted(16, 16, -16, -16),
                         Qt::AlignTop | Qt::AlignLeft,
                         QStringLiteral("Local %1").arg(sceneFrame_.sourceId));
    } else {
        painter.drawText(rect(), Qt::AlignCenter, QStringLiteral("等待本地媒体或网络帧"));
    }
}
