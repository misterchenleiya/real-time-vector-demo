#include "core/network/udp_device_receiver.h"

#include "core/network/packet_codec.h"

#include <QDateTime>
#include <QNetworkDatagram>

namespace demo {

UdpDeviceReceiver::UdpDeviceReceiver(QObject *parent)
    : QObject(parent)
{
    connect(&socket_, &QUdpSocket::readyRead, this, &UdpDeviceReceiver::handleReadyRead);
}

bool UdpDeviceReceiver::listen(quint16 port, QString *error)
{
    stop();

    if (!socket_.bind(QHostAddress::AnyIPv4, port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to bind UDP port %1: %2").arg(port).arg(socket_.errorString());
        }
        return false;
    }

    listenPort_ = port;
    return true;
}

void UdpDeviceReceiver::stop()
{
    partialFrames_.clear();
    if (socket_.state() != QAbstractSocket::UnconnectedState) {
        socket_.close();
    }
    listenPort_ = 0;
}

void UdpDeviceReceiver::setFragmentTimeoutMs(int timeoutMs)
{
    fragmentTimeoutMs_ = timeoutMs;
}

quint16 UdpDeviceReceiver::listenPort() const
{
    return listenPort_;
}

QString UdpDeviceReceiver::makeKey(const QString &sourceId, const QString &deviceId, quint64 frameId) const
{
    return QStringLiteral("%1|%2|%3").arg(sourceId, deviceId).arg(frameId);
}

void UdpDeviceReceiver::purgeExpiredFrames()
{
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    for (auto it = partialFrames_.begin(); it != partialFrames_.end();) {
        if ((now - it->lastUpdateMs) > fragmentTimeoutMs_) {
            it = partialFrames_.erase(it);
        } else {
            ++it;
        }
    }
}

void UdpDeviceReceiver::handleReadyRead()
{
    purgeExpiredFrames();

    while (socket_.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = socket_.receiveDatagram();
        FragmentPacket fragment;
        QString error;
        if (!PacketCodec::decodeFragmentDatagram(datagram.data(), &fragment, &error)) {
            emit receiverError(error);
            continue;
        }

        const QString key = makeKey(fragment.sourceId, fragment.deviceId, fragment.frameId);
        PartialFrame &partial = partialFrames_[key];
        if (partial.fragments.isEmpty()) {
            partial.sourceId = fragment.sourceId;
            partial.deviceId = fragment.deviceId;
            partial.frameId = fragment.frameId;
            partial.fragmentCount = fragment.fragmentCount;
            partial.fragments.resize(fragment.fragmentCount);
        }

        if (fragment.fragmentIndex >= partial.fragments.size()) {
            emit receiverError(QStringLiteral("Received an out-of-range UDP fragment index."));
            partialFrames_.remove(key);
            continue;
        }

        partial.fragments[fragment.fragmentIndex] = fragment.payload;
        partial.lastUpdateMs = QDateTime::currentMSecsSinceEpoch();

        bool complete = true;
        for (const QByteArray &payload : partial.fragments) {
            if (payload.isEmpty()) {
                complete = false;
                break;
            }
        }
        if (!complete) {
            continue;
        }

        QByteArray merged;
        for (const QByteArray &payload : partial.fragments) {
            merged.append(payload);
        }

        DeviceFrame frame;
        if (!PacketCodec::decodeFrame(merged, &frame, &error)) {
            emit receiverError(error);
            partialFrames_.remove(key);
            continue;
        }

        partialFrames_.remove(key);
        emit frameReceived(frame);
    }
}

}  // namespace demo
