#include "core/processing/sender_engine.h"

#include <QDateTime>

namespace demo {

SenderEngine::SenderEngine(QObject *parent)
    : QObject(parent)
{
    connect(&timer_, &QTimer::timeout, this, &SenderEngine::processTick);
}

bool SenderEngine::configure(const AppConfig &config, QString *error)
{
    Q_UNUSED(error)
    config_ = config;
    configured_ = true;
    return true;
}

bool SenderEngine::loadMedia(const QString &path, QString *error)
{
    if (!configured_) {
        if (error != nullptr) {
            *error = QStringLiteral("The sender engine has not been configured yet.");
        }
        return false;
    }
    return mediaProcessor_.open(path, config_.processing, config_.network.sourceId, error);
}

void SenderEngine::clearMedia()
{
    stop();
    mediaProcessor_.close();
}

bool SenderEngine::hasMedia() const
{
    return mediaProcessor_.isOpen();
}

QString SenderEngine::mediaPath() const
{
    return mediaProcessor_.mediaPath();
}

void SenderEngine::start()
{
    if (!configured_) {
        emit errorOccurred(QStringLiteral("The sender engine is not configured."));
        return;
    }
    if (!mediaProcessor_.isOpen()) {
        emit errorOccurred(QStringLiteral("No local image or video is loaded."));
        return;
    }

    const int intervalMs = std::max(1, 1000 / std::max(1, config_.processing.targetFps));
    timer_.start(intervalMs);
    resetMetricsWindow();
    emit stateMessage(QStringLiteral("Sender engine started."));
}

void SenderEngine::stop()
{
    const bool wasRunning = timer_.isActive();
    if (!wasRunning) {
        return;
    }

    timer_.stop();
    sendClearFrames();
    emit stateMessage(QStringLiteral("Sender engine stopped."));
}

bool SenderEngine::isRunning() const
{
    return timer_.isActive();
}

EngineMetrics SenderEngine::metrics() const
{
    return metrics_;
}

void SenderEngine::resetMetricsWindow()
{
    metricsWindowStartMs_ = QDateTime::currentMSecsSinceEpoch();
    metricsWindowProcessed_ = 0;
    metricsWindowSent_ = 0;
}

void SenderEngine::sendClearFrames()
{
    if (!configured_) {
        return;
    }

    QString error;
    const quint64 clearFrameId = metrics_.framesProcessed + 1;
    for (const DeviceTarget &target : config_.network.targets) {
        DeviceFrame frame;
        frame.sourceId = config_.network.sourceId;
        frame.deviceId = target.id;
        frame.deviceName = target.name;
        frame.zoneId = target.zoneId;
        frame.frameId = clearFrameId;
        frame.timestampUs = currentTimestampMicros();
        frame.sourceSize = QSize(config_.processing.processingWidth, config_.processing.processingHeight);
        frame.clearRequested = true;

        if (!sender_.sendFrame(frame, target.host, target.port, config_.network.maxPacketSize, &error)) {
            emit errorOccurred(error);
        }
    }
}

void SenderEngine::processTick()
{
    MediaProcessor::Output output;
    QString error;
    if (!mediaProcessor_.next(&output, &error)) {
        emit errorOccurred(error);
        stop();
        return;
    }

    emit previewReady(output.previewImage);
    emit sceneReady(output.sceneFrame);

    const QVector<DeviceFrame> routedFrames = router_.route(output.sceneFrame, config_);
    bool emittedFirstRoutedFrame = false;
    for (const DeviceFrame &frame : routedFrames) {
        const DeviceTarget *target = nullptr;
        for (const DeviceTarget &candidate : config_.network.targets) {
            if (candidate.id == frame.deviceId) {
                target = &candidate;
                break;
            }
        }
        if (target == nullptr) {
            continue;
        }

        if (!sender_.sendFrame(frame, target->host, target->port, config_.network.maxPacketSize, &error)) {
            emit errorOccurred(error);
            continue;
        }

        if (!emittedFirstRoutedFrame) {
            emit routedFrameReady(frame);
            emittedFirstRoutedFrame = true;
        }

        metrics_.framesSent += 1;
        metricsWindowSent_ += 1;
    }

    metrics_.framesProcessed += 1;
    metrics_.lastPathCount = output.sceneFrame.paths.size();
    metrics_.lastDeviceCount = routedFrames.size();
    metrics_.lastFrameLatencyMs = (QDateTime::currentMSecsSinceEpoch() * 1000 - output.sceneFrame.timestampUs) / 1000;
    metricsWindowProcessed_ += 1;

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsedMs = nowMs - metricsWindowStartMs_;
    if (elapsedMs >= 1000) {
        metrics_.processingFps = (metricsWindowProcessed_ * 1000.0) / static_cast<double>(elapsedMs);
        metrics_.sendFps = (metricsWindowSent_ * 1000.0) / static_cast<double>(elapsedMs);
        resetMetricsWindow();
    }

    emit metricsUpdated(metrics_);
}

}  // namespace demo
