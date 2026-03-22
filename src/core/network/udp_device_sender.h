#pragma once

#include "core/model/scene_types.h"

#include <QObject>
#include <QUdpSocket>

namespace demo {

class UdpDeviceSender : public QObject {
    Q_OBJECT

public:
    explicit UdpDeviceSender(QObject *parent = nullptr);

    bool sendFrame(const DeviceFrame &frame, const QString &host, quint16 port, int maxPacketSize, QString *error);
};

}  // namespace demo
