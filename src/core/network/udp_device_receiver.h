#pragma once

#include "core/model/scene_types.h"

#include <QObject>
#include <QHash>
#include <QUdpSocket>
#include <QVector>

namespace demo {

class UdpDeviceReceiver : public QObject {
    Q_OBJECT

public:
    explicit UdpDeviceReceiver(QObject *parent = nullptr);

    bool listen(quint16 port, QString *error);
    void stop();
    void setFragmentTimeoutMs(int timeoutMs);
    quint16 listenPort() const;

signals:
    void frameReceived(const demo::DeviceFrame &frame);
    void receiverError(const QString &message);

private slots:
    void handleReadyRead();

private:
    struct PartialFrame {
        QString sourceId;
        QString deviceId;
        quint64 frameId = 0;
        quint16 fragmentCount = 0;
        QVector<QByteArray> fragments;
        qint64 lastUpdateMs = 0;
    };

    QString makeKey(const QString &sourceId, const QString &deviceId, quint64 frameId) const;
    void purgeExpiredFrames();

    QUdpSocket socket_;
    quint16 listenPort_ = 0;
    int fragmentTimeoutMs_ = 200;
    QHash<QString, PartialFrame> partialFrames_;
};

}  // namespace demo
