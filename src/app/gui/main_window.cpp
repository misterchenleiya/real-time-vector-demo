#include "app/gui/main_window.h"

#include <QApplication>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextEdit>
#include <QVBoxLayout>

MainWindow::MainWindow(const QString &initialConfigPath, QWidget *parent)
    : QMainWindow(parent)
{
    qRegisterMetaType<demo::SceneFrame>();
    qRegisterMetaType<demo::DeviceFrame>();
    qRegisterMetaType<demo::EngineMetrics>();

    buildUi();

    connect(&receiver_, &demo::UdpDeviceReceiver::frameReceived, this, &MainWindow::handleRemoteFrame);
    connect(&receiver_, &demo::UdpDeviceReceiver::receiverError, this, &MainWindow::handleRemoteError);
    connect(&sender_, &demo::SenderEngine::previewReady, this, &MainWindow::handleSenderPreview);
    connect(&sender_, &demo::SenderEngine::sceneReady, this, &MainWindow::handleSenderScene);
    connect(&sender_, &demo::SenderEngine::metricsUpdated, this, &MainWindow::handleSenderMetrics);
    connect(&sender_, &demo::SenderEngine::errorOccurred, this, &MainWindow::handleSenderError);
    connect(&sender_, &demo::SenderEngine::stateMessage, this, &MainWindow::handleStateMessage);

    const QString configToLoad = initialConfigPath.isEmpty() ? defaultConfigPath() : initialConfigPath;
    if (!configToLoad.isEmpty()) {
        loadConfigFile(configToLoad);
    } else {
        logMessage(QStringLiteral("未找到默认配置，请先手动加载 JSON 配置文件"));
    }

    updateStatusLabels();
}

void MainWindow::buildUi()
{
    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    auto *controlsLayout = new QHBoxLayout();
    auto *loadConfigButton = new QPushButton(QStringLiteral("加载配置"), this);
    auto *loadMediaButton = new QPushButton(QStringLiteral("加载媒体"), this);
    auto *clearMediaButton = new QPushButton(QStringLiteral("清空媒体"), this);
    auto *startButton = new QPushButton(QStringLiteral("开始"), this);
    auto *stopButton = new QPushButton(QStringLiteral("停止"), this);

    controlsLayout->addWidget(loadConfigButton);
    controlsLayout->addWidget(loadMediaButton);
    controlsLayout->addWidget(clearMediaButton);
    controlsLayout->addStretch(1);
    controlsLayout->addWidget(startButton);
    controlsLayout->addWidget(stopButton);
    rootLayout->addLayout(controlsLayout);

    auto *statusGroup = new QGroupBox(QStringLiteral("运行状态"), this);
    auto *statusLayout = new QFormLayout(statusGroup);
    modeLabel_ = new QLabel(QStringLiteral("待机"), this);
    configLabel_ = new QLabel(QStringLiteral("-"), this);
    mediaLabel_ = new QLabel(QStringLiteral("-"), this);
    networkLabel_ = new QLabel(QStringLiteral("-"), this);
    metricsLabel_ = new QLabel(QStringLiteral("-"), this);
    statusLayout->addRow(QStringLiteral("模式"), modeLabel_);
    statusLayout->addRow(QStringLiteral("配置"), configLabel_);
    statusLayout->addRow(QStringLiteral("媒体"), mediaLabel_);
    statusLayout->addRow(QStringLiteral("网络"), networkLabel_);
    statusLayout->addRow(QStringLiteral("指标"), metricsLabel_);

    auto *paramsGroup = new QGroupBox(QStringLiteral("关键参数"), this);
    auto *paramsLayout = new QFormLayout(paramsGroup);
    targetFpsSpin_ = new QSpinBox(this);
    targetFpsSpin_->setRange(1, 120);
    maxPathsSpin_ = new QSpinBox(this);
    maxPathsSpin_->setRange(1, 32);
    minAreaRatioSpin_ = new QDoubleSpinBox(this);
    minAreaRatioSpin_->setRange(0.0001, 0.2);
    minAreaRatioSpin_->setDecimals(4);
    minAreaRatioSpin_->setSingleStep(0.0005);
    stabilityBlendSpin_ = new QDoubleSpinBox(this);
    stabilityBlendSpin_->setRange(0.0, 0.98);
    stabilityBlendSpin_->setDecimals(2);
    stabilityBlendSpin_->setSingleStep(0.05);
    paramsLayout->addRow(QStringLiteral("目标 FPS"), targetFpsSpin_);
    paramsLayout->addRow(QStringLiteral("最大路径数"), maxPathsSpin_);
    paramsLayout->addRow(QStringLiteral("最小面积比"), minAreaRatioSpin_);
    paramsLayout->addRow(QStringLiteral("稳定混合"), stabilityBlendSpin_);

    auto *topLayout = new QHBoxLayout();
    topLayout->addWidget(statusGroup, 2);
    topLayout->addWidget(paramsGroup, 1);
    rootLayout->addLayout(topLayout);

    previewWidget_ = new PreviewWidget(this);
    rootLayout->addWidget(previewWidget_, 1);

    logView_ = new QTextEdit(this);
    logView_->setReadOnly(true);
    logView_->setMinimumHeight(140);
    rootLayout->addWidget(logView_);

    setCentralWidget(central);
    resize(1180, 900);
    setWindowTitle(QStringLiteral("DEMO"));

    connect(loadConfigButton, &QPushButton::clicked, this, &MainWindow::chooseConfigFile);
    connect(loadMediaButton, &QPushButton::clicked, this, &MainWindow::chooseMediaFile);
    connect(clearMediaButton, &QPushButton::clicked, this, &MainWindow::clearMediaSelection);
    connect(startButton, &QPushButton::clicked, this, &MainWindow::startWorkflow);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::stopWorkflow);
}

QString MainWindow::defaultConfigPath() const
{
    const QStringList candidates = {
        QDir::current().absoluteFilePath(QStringLiteral("configs/demo.receiver.50010.json")),
        QDir::current().absoluteFilePath(QStringLiteral("configs/demo.sender.json")),
        QDir(QApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../configs/demo.receiver.50010.json")),
        QDir(QApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../configs/demo.sender.json"))
    };

    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return QFileInfo(candidate).canonicalFilePath();
        }
    }
    return {};
}

void MainWindow::chooseConfigFile()
{
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择配置文件"),
        QDir::currentPath(),
        QStringLiteral("JSON (*.json)"));
    if (!path.isEmpty()) {
        loadConfigFile(path);
    }
}

void MainWindow::chooseMediaFile()
{
    const QString startDir = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
    const QString path = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("选择图片或视频"),
        startDir,
        QStringLiteral("Media (*.mp4 *.mov *.avi *.mkv *.png *.jpg *.jpeg *.bmp *.webp)"));
    if (path.isEmpty()) {
        return;
    }

    mediaPath_ = path;
    mediaLabel_->setText(path);
    logMessage(QStringLiteral("已选择本地媒体: %1").arg(path));
    updateStatusLabels();
}

void MainWindow::clearMediaSelection()
{
    sender_.clearMedia();
    mediaPath_.clear();
    previewWidget_->clearPreview();
    logMessage(QStringLiteral("已清空本地媒体，回到接收待机模式"));
    startReceiverMode();
    updateStatusLabels();
}

void MainWindow::startWorkflow()
{
    applyUiParametersToConfig();
    stopReceiverMode();

    if (mediaPath_.isEmpty()) {
        logMessage(QStringLiteral("当前没有本地媒体，自动进入接收模式"));
        startReceiverMode();
        return;
    }

    QString error;
    if (!sender_.configure(config_, &error)) {
        logMessage(error);
        return;
    }

    if (!sender_.loadMedia(mediaPath_, &error)) {
        logMessage(error);
        startReceiverMode();
        return;
    }

    sender_.start();
    receiverModeActive_ = false;
    updateStatusLabels();
}

void MainWindow::stopWorkflow()
{
    sender_.stop();
    if (mediaPath_.isEmpty()) {
        startReceiverMode();
    }
    updateStatusLabels();
}

void MainWindow::handleRemoteFrame(const demo::DeviceFrame &frame)
{
    if (sender_.isRunning()) {
        return;
    }

    previewWidget_->setDeviceFrame(frame);
    networkLabel_->setText(QStringLiteral("监听 %1，最近源 %2 -> %3")
                               .arg(config_.network.listenPort)
                               .arg(frame.sourceId, frame.deviceId));
    modeLabel_->setText(QStringLiteral("接收中"));
}

void MainWindow::handleRemoteError(const QString &message)
{
    logMessage(QStringLiteral("接收错误: %1").arg(message));
}

void MainWindow::handleSenderPreview(const QImage &image)
{
    previewWidget_->setSourceImage(image);
}

void MainWindow::handleSenderScene(const demo::SceneFrame &frame)
{
    previewWidget_->setSceneFrame(frame);
    modeLabel_->setText(QStringLiteral("发送中"));
}

void MainWindow::handleSenderMetrics(const demo::EngineMetrics &metrics)
{
    metricsLabel_->setText(
        QStringLiteral("处理 %1 FPS | 发送 %2 FPS | paths=%3 | devices=%4 | latency=%5ms")
            .arg(metrics.processingFps, 0, 'f', 1)
            .arg(metrics.sendFps, 0, 'f', 1)
            .arg(metrics.lastPathCount)
            .arg(metrics.lastDeviceCount)
            .arg(metrics.lastFrameLatencyMs));
}

void MainWindow::handleSenderError(const QString &message)
{
    logMessage(QStringLiteral("发送错误: %1").arg(message));
}

void MainWindow::handleStateMessage(const QString &message)
{
    logMessage(message);
    updateStatusLabels();
}

void MainWindow::loadConfigFile(const QString &path)
{
    demo::AppConfig loaded;
    QString error;
    if (!demo::AppConfigLoader::loadFromFile(path, &loaded, &error)) {
        logMessage(error);
        return;
    }

    config_ = loaded;
    configPath_ = path;
    configLabel_->setText(path);
    targetFpsSpin_->setValue(config_.processing.targetFps);
    maxPathsSpin_->setValue(config_.processing.maxPaths);
    minAreaRatioSpin_->setValue(config_.processing.minAreaRatio);
    stabilityBlendSpin_->setValue(config_.processing.stabilityBlend);
    logMessage(QStringLiteral("已加载配置: %1").arg(path));

    if (mediaPath_.isEmpty() && !sender_.isRunning()) {
        startReceiverMode();
    } else {
        updateStatusLabels();
    }
}

void MainWindow::startReceiverMode()
{
    if (config_.network.listenPort == 0) {
        logMessage(QStringLiteral("当前配置未设置 listenPort，无法进入接收模式"));
        return;
    }

    QString error;
    receiver_.setFragmentTimeoutMs(config_.network.fragmentTimeoutMs);
    if (!receiver_.listen(config_.network.listenPort, &error)) {
        logMessage(error);
        receiverModeActive_ = false;
        updateStatusLabels();
        return;
    }

    receiverModeActive_ = true;
    networkLabel_->setText(QStringLiteral("监听 127.0.0.1:%1").arg(config_.network.listenPort));
    modeLabel_->setText(QStringLiteral("接收待机"));
    logMessage(QStringLiteral("已进入接收模式，监听端口 %1").arg(config_.network.listenPort));
}

void MainWindow::stopReceiverMode()
{
    if (receiverModeActive_) {
        receiver_.stop();
        receiverModeActive_ = false;
    }
}

void MainWindow::applyUiParametersToConfig()
{
    config_.processing.targetFps = targetFpsSpin_->value();
    config_.processing.maxPaths = maxPathsSpin_->value();
    config_.processing.minAreaRatio = minAreaRatioSpin_->value();
    config_.processing.stabilityBlend = stabilityBlendSpin_->value();
}

void MainWindow::updateStatusLabels()
{
    if (sender_.isRunning()) {
        modeLabel_->setText(QStringLiteral("发送中"));
    } else if (receiverModeActive_) {
        modeLabel_->setText(QStringLiteral("接收待机"));
    } else {
        modeLabel_->setText(QStringLiteral("待机"));
    }

    configLabel_->setText(configPath_.isEmpty() ? QStringLiteral("-") : configPath_);
    mediaLabel_->setText(mediaPath_.isEmpty() ? QStringLiteral("(无)") : mediaPath_);

    if (!sender_.isRunning() && !receiverModeActive_) {
        networkLabel_->setText(QStringLiteral("未监听"));
    }
}

void MainWindow::logMessage(const QString &message)
{
    logView_->append(QStringLiteral("[%1] %2")
                         .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")))
                         .arg(message));
}
