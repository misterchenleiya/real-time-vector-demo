#pragma once

#include "app/gui/preview_widget.h"
#include "core/config/app_config.h"
#include "core/network/udp_device_receiver.h"
#include "core/processing/sender_engine.h"

#include <QMainWindow>

class QLabel;
class QTextEdit;
class QPushButton;
class QSpinBox;
class QDoubleSpinBox;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString &initialConfigPath = {}, QWidget *parent = nullptr);

private slots:
    void chooseConfigFile();
    void chooseMediaFile();
    void clearMediaSelection();
    void startWorkflow();
    void stopWorkflow();

    void handleRemoteFrame(const demo::DeviceFrame &frame);
    void handleRemoteError(const QString &message);
    void handleSenderPreview(const QImage &image);
    void handleSenderScene(const demo::SceneFrame &frame);
    void handleSenderMetrics(const demo::EngineMetrics &metrics);
    void handleSenderError(const QString &message);
    void handleStateMessage(const QString &message);

private:
    void buildUi();
    void loadConfigFile(const QString &path);
    void startReceiverMode();
    void stopReceiverMode();
    void applyUiParametersToConfig();
    void updateStatusLabels();
    void logMessage(const QString &message);
    QString defaultConfigPath() const;

    demo::AppConfig config_;
    QString configPath_;
    QString mediaPath_;
    bool receiverModeActive_ = false;

    demo::UdpDeviceReceiver receiver_;
    demo::SenderEngine sender_;

    PreviewWidget *previewWidget_ = nullptr;
    QLabel *modeLabel_ = nullptr;
    QLabel *configLabel_ = nullptr;
    QLabel *mediaLabel_ = nullptr;
    QLabel *networkLabel_ = nullptr;
    QLabel *metricsLabel_ = nullptr;
    QTextEdit *logView_ = nullptr;
    QSpinBox *targetFpsSpin_ = nullptr;
    QSpinBox *maxPathsSpin_ = nullptr;
    QDoubleSpinBox *minAreaRatioSpin_ = nullptr;
    QDoubleSpinBox *stabilityBlendSpin_ = nullptr;
};
