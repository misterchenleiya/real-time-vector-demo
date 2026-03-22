#pragma once

#include "core/model/scene_types.h"

#include <QByteArray>
#include <QString>
#include <QVector>

namespace demo {

struct FragmentPacket {
    QString sourceId;
    QString deviceId;
    quint64 frameId = 0;
    quint16 fragmentIndex = 0;
    quint16 fragmentCount = 0;
    QByteArray payload;
};

class PacketCodec {
public:
    static QByteArray encodeFrame(const DeviceFrame &frame);
    static bool decodeFrame(const QByteArray &bytes, DeviceFrame *frame, QString *error);

    static QVector<QByteArray> fragmentFrame(const DeviceFrame &frame, int maxPacketSize, QString *error);
    static bool decodeFragmentDatagram(const QByteArray &datagram, FragmentPacket *fragment, QString *error);
};

}  // namespace demo
