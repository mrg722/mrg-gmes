#pragma once
#include "raylib.h"

namespace district_fury {

class Scene {
public:
    void Init();
    void DrawBackground(float cameraX) const;
    void DrawForeground(float cameraX) const;
};

}
