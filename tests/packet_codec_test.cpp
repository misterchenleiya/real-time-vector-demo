#include "core/model/scene_types.h"
#include "core/network/packet_codec.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    demo::DeviceFrame frame;
    frame.sourceId = QStringLiteral("sender-a");
    frame.deviceId = QStringLiteral("device-1");
    frame.deviceName = QStringLiteral("Receiver A");
    frame.zoneId = QStringLiteral("full");
    frame.frameId = 42;
    frame.timestampUs = 123456789;
    frame.sourceSize = QSize(640, 360);

    demo::PathStroke path;
    path.id = QStringLiteral("path-1");
    for (int index = 0; index < 96; ++index) {
        const qreal t = static_cast<qreal>(index) / 95.0;
        path.points.push_back(QPointF(t, 0.15 + (0.7 * t)));
    }
    frame.paths.push_back(path);

    QString error;
    const QByteArray encoded = demo::PacketCodec::encodeFrame(frame);
    demo::DeviceFrame decoded;
    if (!demo::PacketCodec::decodeFrame(encoded, &decoded, &error)) {
        qCritical().noquote() << error;
        return 1;
    }

    if (decoded.sourceId != frame.sourceId || decoded.deviceId != frame.deviceId || decoded.paths.size() != 1) {
        qCritical() << "设备帧 round-trip 校验失败";
        return 1;
    }

    const QVector<QByteArray> fragments = demo::PacketCodec::fragmentFrame(frame, 320, &error);
    if (fragments.size() < 2) {
        qCritical() << "预期应该产生多个 UDP 分片";
        return 1;
    }

    for (int index = 0; index < fragments.size(); ++index) {
        demo::FragmentPacket fragment;
        if (!demo::PacketCodec::decodeFragmentDatagram(fragments[index], &fragment, &error)) {
            qCritical().noquote() << error;
            return 1;
        }

        if (fragment.frameId != frame.frameId || fragment.fragmentIndex != index) {
            qCritical() << "UDP 分片头校验失败";
            return 1;
        }
    }

    return 0;
}
