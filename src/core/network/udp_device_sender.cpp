#include "core/network/udp_device_sender.h"

#include "core/network/packet_codec.h"

namespace demo {

UdpDeviceSender::UdpDeviceSender(QObject *parent)
    : QObject(parent)
{
}

bool UdpDeviceSender::sendFrame(const DeviceFrame &frame, const QString &host, quint16 port, int maxPacketSize, QString *error)
{
    const QVector<QByteArray> datagrams = PacketCodec::fragmentFrame(frame, maxPacketSize, error);
    if (datagrams.isEmpty()) {
        return false;
    }

    QUdpSocket socket;
    for (const QByteArray &datagram : datagrams) {
        const qint64 written = socket.writeDatagram(datagram, QHostAddress(host), port);
        if (written != datagram.size()) {
            if (error != nullptr) {
                *error = QStringLiteral("UDP 发送失败: %1").arg(socket.errorString());
            }
            return false;
        }
    }

    return true;
}

}  // namespace demo
