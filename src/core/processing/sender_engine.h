#pragma once

#include "core/config/app_config.h"
#include "core/model/scene_types.h"
#include "core/network/udp_device_sender.h"
#include "core/processing/media_processor.h"
#include "core/routing/frame_router.h"

#include <QObject>
#include <QTimer>

namespace demo {

class SenderEngine : public QObject {
    Q_OBJECT

public:
    explicit SenderEngine(QObject *parent = nullptr);

    bool configure(const AppConfig &config, QString *error);
    bool loadMedia(const QString &path, QString *error);
    void clearMedia();
    bool hasMedia() const;
    QString mediaPath() const;

    void start();
    void stop();
    bool isRunning() const;
    EngineMetrics metrics() const;

signals:
    void previewReady(const QImage &image);
    void sceneReady(const demo::SceneFrame &frame);
    void routedFrameReady(const demo::DeviceFrame &frame);
    void metricsUpdated(const demo::EngineMetrics &metrics);
    void errorOccurred(const QString &message);
    void stateMessage(const QString &message);

private slots:
    void processTick();

private:
    void resetMetricsWindow();

    QTimer timer_;
    AppConfig config_;
    bool configured_ = false;
    MediaProcessor mediaProcessor_;
    FrameRouter router_;
    UdpDeviceSender sender_;
    EngineMetrics metrics_;
    qint64 metricsWindowStartMs_ = 0;
    quint64 metricsWindowProcessed_ = 0;
    quint64 metricsWindowSent_ = 0;
};

}  // namespace demo
