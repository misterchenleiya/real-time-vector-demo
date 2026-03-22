#include "core/network/packet_codec.h"

#include <QBuffer>
#include <QDataStream>

namespace demo {

namespace {

constexpr quint32 kFrameMagic = 0x44464D31;
constexpr quint16 kFrameVersion = 1;
constexpr quint32 kFragmentMagic = 0x44504731;
constexpr quint16 kFragmentVersion = 1;

}  // namespace

QByteArray PacketCodec::encodeFrame(const DeviceFrame &frame)
{
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << kFrameMagic << kFrameVersion << frame;
    return bytes;
}

bool PacketCodec::decodeFrame(const QByteArray &bytes, DeviceFrame *frame, QString *error)
{
    if (frame == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("The output frame object is null.");
        }
        return false;
    }

    QDataStream stream(bytes);
    stream.setVersion(QDataStream::Qt_5_15);
    quint32 magic = 0;
    quint16 version = 0;
    stream >> magic >> version;
    if (magic != kFrameMagic || version != kFrameVersion) {
        if (error != nullptr) {
            *error = QStringLiteral("The device frame header does not match the protocol.");
        }
        return false;
    }

    DeviceFrame decoded;
    stream >> decoded;
    if (stream.status() != QDataStream::Ok || !decoded.isValid()) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to decode the device frame.");
        }
        return false;
    }

    *frame = decoded;
    return true;
}

QVector<QByteArray> PacketCodec::fragmentFrame(const DeviceFrame &frame, int maxPacketSize, QString *error)
{
    QVector<QByteArray> datagrams;
    const QByteArray payload = encodeFrame(frame);
    if (payload.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("The encoded device frame is empty.");
        }
        return datagrams;
    }

    const int usablePayloadSize = std::max(256, maxPacketSize - 256);
    const int fragmentCount = std::max(1, (payload.size() + usablePayloadSize - 1) / usablePayloadSize);

    for (int index = 0; index < fragmentCount; ++index) {
        const QByteArray chunk = payload.mid(index * usablePayloadSize, usablePayloadSize);
        QByteArray datagram;
        QDataStream stream(&datagram, QIODevice::WriteOnly);
        stream.setVersion(QDataStream::Qt_5_15);
        stream << kFragmentMagic
               << kFragmentVersion
               << frame.sourceId
               << frame.deviceId
               << frame.frameId
               << static_cast<quint16>(index)
               << static_cast<quint16>(fragmentCount)
               << chunk;
        datagrams.push_back(datagram);
    }

    return datagrams;
}

bool PacketCodec::decodeFragmentDatagram(const QByteArray &datagram, FragmentPacket *fragment, QString *error)
{
    if (fragment == nullptr) {
        if (error != nullptr) {
            *error = QStringLiteral("The output fragment object is null.");
        }
        return false;
    }

    QDataStream stream(datagram);
    stream.setVersion(QDataStream::Qt_5_15);

    quint32 magic = 0;
    quint16 version = 0;
    FragmentPacket decoded;
    stream >> magic
           >> version
           >> decoded.sourceId
           >> decoded.deviceId
           >> decoded.frameId
           >> decoded.fragmentIndex
           >> decoded.fragmentCount
           >> decoded.payload;

    if (magic != kFragmentMagic || version != kFragmentVersion || decoded.fragmentCount == 0) {
        if (error != nullptr) {
            *error = QStringLiteral("The UDP fragment header does not match the protocol.");
        }
        return false;
    }

    if (stream.status() != QDataStream::Ok || decoded.payload.isEmpty()) {
        if (error != nullptr) {
            *error = QStringLiteral("Failed to decode the UDP fragment.");
        }
        return false;
    }

    *fragment = decoded;
    return true;
}

}  // namespace demo
