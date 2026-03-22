#pragma once

#include "core/config/app_config.h"
#include "core/model/scene_types.h"

#include <QImage>
#include <QString>
#include <QVector>

namespace demo {

class MediaProcessor {
public:
    struct Output {
        QImage previewImage;
        SceneFrame sceneFrame;
    };

    bool open(const QString &path, const ProcessingConfig &config, const QString &sourceId, QString *error);
    void close();
    bool isOpen() const;
    QString mediaPath() const;
    bool next(Output *output, QString *error);

private:
    struct StablePathState {
        QString id;
        QColor color = Qt::white;
        QPointF centroid;
        QVector<QPointF> points;
    };

    QVector<PathStroke> stabilizePaths(const QVector<PathStroke> &paths);
    QVector<QPointF> resampleClosedPath(const QVector<QPointF> &points) const;
    QVector<QPointF> smoothPoints(const QVector<QPointF> &points) const;

#if DEMO_HAS_OPENCV
    bool openImage(const QString &path, QString *error);
    bool openVideo(const QString &path, QString *error);
    bool readNextFrame(QImage *previewImage, SceneFrame *sceneFrame, QString *error);
#endif

    QString mediaPath_;
    QString sourceId_;
    ProcessingConfig config_;
    quint64 nextFrameId_ = 1;
    quint64 nextPathId_ = 1;
    QVector<StablePathState> previousPaths_;

#if DEMO_HAS_OPENCV
    bool stillImageMode_ = false;
    class Impl;
    Impl *impl_ = nullptr;
#endif
};

}  // namespace demo
