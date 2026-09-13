#include "game/Scene.h"
#include "rendering/AssetManager.h"
#include "raylib.h"

namespace district_fury {

void Scene::Init() {
}

void Scene::DrawBackground() const {
    Texture2D bg = AssetManager::Get().GetTexture("bg_industrial");
    if (bg.id != 0) {
        // Draw background scaled to fit width, top aligned
        float scale = 1280.0f / bg.width;
        DrawTextureEx(bg, {0, -200.0f}, 0.0f, scale, WHITE);
        
        // Darken the background slightly for atmosphere
        DrawRectangle(0, 0, 1280, 720, {0, 20, 10, 80});
        
        // Draw floor area
        DrawRectangleGradientV(0, 450, 1280, 270, {10, 15, 10, 150}, {0, 0, 0, 220});
        
        // Floor lines for perspective / depth
        for (int i = 0; i < 5; i++) {
            DrawLine(0, 450 + i * 50, 1280, 450 + i * 50, {30, 45, 35, 150});
        }
    } else {
        ClearBackground({20, 25, 25, 255});
    }
}

void Scene::DrawForeground() const {
    DrawRectangleGradientV(0, 0, 1280, 100, {0,0,0,150}, {0,0,0,0});
    DrawRectangleGradientV(0, 620, 1280, 100, {0,0,0,0}, {0,0,0,150});
}

}
