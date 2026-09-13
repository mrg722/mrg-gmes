#pragma once
#include "raylib.h"

namespace district_fury {
class Scene {
public:
    void Init();
    void DrawBackground() const;
    void DrawForeground() const;
};
}
