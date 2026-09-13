#include "game/Scene.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>

namespace district_fury {

void Scene::Init() {
}

void Scene::DrawBackground() const {
    const Texture2D bg = AssetManager::Get().GetTexture("bg_industrial");
    if (bg.id == 0) {
        ClearBackground({12, 16, 15, 255});
        return;
    }

    const float scale = std::max(1280.0f / static_cast<float>(bg.width), 720.0f / static_cast<float>(bg.height));
    const float drawWidth = bg.width * scale;
    const float drawHeight = bg.height * scale;
    const float x = (1280.0f - drawWidth) * 0.5f;
    const float y = (720.0f - drawHeight) * 0.5f;
    DrawTextureEx(bg, {x, y}, 0.0f, scale, WHITE);

    // Deepen the industrial atmosphere without obscuring the generated artwork.
    DrawRectangleGradientV(0, 0, 1280, 720, {4, 8, 8, 72}, {0, 0, 0, 115});
    DrawCircleGradient(200, 420, 180.0f, {60, 255, 100, 34}, {0, 0, 0, 0});
    DrawCircleGradient(930, 390, 230.0f, {70, 255, 110, 26}, {0, 0, 0, 0});

    // Playable floor: a subtle perspective grid anchors the characters to the yard.
    DrawRectangleGradientV(0, 420, 1280, 300, {11, 18, 18, 90}, {4, 8, 8, 205});
    for (int i = 0; i < 7; ++i) {
        const int yLine = 445 + i * 42;
        DrawLine(0, yLine, 1280, yLine, {45, 65, 58, 92});
    }
    for (int x = -200; x <= 1500; x += 180) {
        DrawLine(640, 420, x, 720, {45, 65, 58, 48});
    }

    // Toxic light pools in the distance.
    DrawRectangleGradientH(0, 435, 360, 120, {30, 220, 80, 28}, {30, 220, 80, 0});
    DrawRectangleGradientH(920, 430, 360, 130, {30, 220, 80, 0}, {30, 220, 80, 28});
}

void Scene::DrawForeground() const {
    DrawRectangleGradientV(0, 0, 1280, 95, {0, 0, 0, 135}, {0, 0, 0, 0});
    DrawRectangleGradientV(0, 625, 1280, 95, {0, 0, 0, 0}, {0, 0, 0, 170});
    DrawLine(0, 625, 1280, 625, {100, 135, 120, 35});
}

}
