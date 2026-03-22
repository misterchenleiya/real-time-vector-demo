#pragma once

#include "core/config/app_config.h"
#include "core/model/scene_types.h"

#include <QVector>

namespace demo {

class FrameRouter {
public:
    QVector<DeviceFrame> route(const SceneFrame &sceneFrame, const AppConfig &config) const;
};

}  // namespace demo
