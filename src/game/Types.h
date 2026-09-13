#pragma once
#include "raylib.h"

namespace district_fury {
    struct Vector3D {
        float x = 0.0f; // screen horizontal
        float y = 0.0f; // depth (lane)
        float z = 0.0f; // altitude (jump/air)
        
        Vector2 ToScreen() const {
            return { x, y - z };
        }
    };
    
    enum class Facing {
        Left = -1,
        Right = 1
    };
}
