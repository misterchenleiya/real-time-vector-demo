#pragma once

#include "core/model/scene_types.h"

#include <QImage>
#include <QWidget>

class PreviewWidget : public QWidget {
    Q_OBJECT

public:
    explicit PreviewWidget(QWidget *parent = nullptr);

    void setSourceImage(const QImage &image);
    void setSceneFrame(const demo::SceneFrame &frame);
    void setDeviceFrame(const demo::DeviceFrame &frame);
    void clearPreview();

    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawPaths(QPainter *painter, const QRectF &rect) const;
    QRectF contentRect() const;

    QImage sourceImage_;
    demo::SceneFrame sceneFrame_;
    demo::DeviceFrame deviceFrame_;
};
