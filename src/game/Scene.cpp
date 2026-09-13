#include "game/Scene.h"
#include "game/Types.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {

float TileWorldX(float cameraX, float tileIndex, float parallax) {
    constexpr float tileWidth = kViewportWidth;
    return cameraX * (1.0f - parallax) + tileIndex * tileWidth;
}

void DrawFallbackBuilding(float x, float y, float width, float height, int index) {
    const int base = 20 + (index % 4) * 5;
    DrawRectangle(
        static_cast<int>(x), static_cast<int>(y - height),
        static_cast<int>(width), static_cast<int>(height),
        {static_cast<unsigned char>(base),
         static_cast<unsigned char>(base + 5),
         static_cast<unsigned char>(base + 10), 255}
    );

    const int rows = std::max(2, static_cast<int>(height / 38.0f));
    const int cols = std::max(2, static_cast<int>(width / 34.0f));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if ((r + c + index) % 4 == 0) continue;
            const int wx = static_cast<int>(x + 10.0f + c * 34.0f);
            const int wy = static_cast<int>(y - height + 18.0f + r * 38.0f);
            if (wx + 9 < x + width && wy + 13 < y) {
                DrawRectangle(wx, wy, 9, 13, {125, 160, 65, 115});
            }
        }
    }
}

void DrawAtmosphere(float cameraX) {
    const int seed = static_cast<int>(std::floor(cameraX / 37.0f));

    // Sparse rain streaks. They are deliberately subtle so combat silhouettes remain clear.
    for (int i = 0; i < 70; ++i) {
        const float x = static_cast<float>((seed * 67 + i * 97) % 1400) + cameraX - 60.0f;
        const float y = static_cast<float>((seed * 31 + i * 43) % 650);
        const float length = 6.0f + static_cast<float>(i % 5) * 2.0f;
        DrawLine(
            static_cast<int>(x), static_cast<int>(y),
            static_cast<int>(x - 3.0f), static_cast<int>(y + length),
            {155, 190, 205, 22}
        );
    }

    // Warm/cool atmospheric haze in the upper industrial skyline.
    for (int i = 0; i < 7; ++i) {
        const float x = TileWorldX(cameraX, static_cast<float>(i - 1), 0.22f) + 240.0f;
        const float bob = std::sin(static_cast<float>(GetTime()) * 0.7f + i) * 5.0f;
        DrawCircleGradient(
            static_cast<int>(x), static_cast<int>(405.0f + bob), 34.0f,
            {120, 155, 190, 18}, {120, 155, 190, 0}
        );
    }
}

void DrawFarLayer(float cameraX) {
    for (int i = -2; i < 9; ++i) {
        const float x = TileWorldX(cameraX, static_cast<float>(i), 0.20f);
        const float height = 90.0f + static_cast<float>((i * 31 + 400) % 130);
        DrawRectangle(static_cast<int>(x), static_cast<int>(430.0f - height), 170,
                      static_cast<int>(height), {11, 25, 31, 145});
        DrawRectangle(static_cast<int>(x + 42.0f), static_cast<int>(425.0f - height), 12,
                      static_cast<int>(height * 0.35f), {18, 32, 37, 145});
    }
}

void DrawMidLayer(float cameraX) {
    for (int i = -2; i < 8; ++i) {
        const float x = TileWorldX(cameraX, static_cast<float>(i), 0.46f);
        DrawRectangle(static_cast<int>(x), 430, 240, 170, {24, 35, 38, 175});
        DrawRectangle(static_cast<int>(x + 20.0f), 452, 200, 7, {145, 81, 38, 125});
        DrawRectangle(static_cast<int>(x + 72.0f), 392, 8, 42, {44, 53, 52, 190});
        DrawRectangle(static_cast<int>(x + 156.0f), 405, 7, 27, {44, 53, 52, 190});
    }
}

void DrawPlayLayer(float cameraX) {
    const float left = cameraX - 900.0f;
    const float width = 3080.0f;
    DrawRectangleGradientV(static_cast<int>(left), 620, static_cast<int>(width), 100,
                           {37, 43, 44, 135}, {8, 12, 15, 210});
    for (int i = -5; i < 18; ++i) {
        const float x = TileWorldX(cameraX, static_cast<float>(i), 1.0f) + 110.0f;
        DrawEllipse(static_cast<int>(x), 672, 44.0f, 5.0f, {95, 145, 153, 34});
        DrawLine(static_cast<int>(x - 28.0f), 675, static_cast<int>(x + 28.0f), 675,
                 {148, 183, 180, 22});
    }
}

}

void Scene::Init() {
}

void Scene::DrawBackground(float cameraX) const {
    const Texture2D background = AssetManager::Get().GetTexture("bg_industrial");

    if (background.id != 0) {
        constexpr float parallax = 0.14f;
        const int firstTile = static_cast<int>(std::floor((cameraX * parallax) / kViewportWidth)) - 1;

        // Repeat the authored 16:9 panel across the long stage while preserving a
        // shallow parallax drift. This replaces the old rectangle-only city skyline.
        for (int i = 0; i < 5; ++i) {
            const float tileIndex = static_cast<float>(firstTile + i);
            const float x = TileWorldX(cameraX, tileIndex, parallax);
            DrawTexturePro(
                background,
                {0.0f, 0.0f, static_cast<float>(background.width), static_cast<float>(background.height)},
                {x, 0.0f, kViewportWidth, kViewportHeight},
                {0.0f, 0.0f}, 0.0f, WHITE
            );
        }

        DrawFarLayer(cameraX);
        DrawMidLayer(cameraX);
        DrawPlayLayer(cameraX);
        DrawAtmosphere(cameraX);
        return;
    }

    // Safe visual fallback if the authored background is missing.
    const float left = cameraX - 900.0f;
    const float width = 3080.0f;
    DrawRectangleGradientV(
        static_cast<int>(left), 0, static_cast<int>(width), 470,
        {7, 12, 18, 255}, {26, 29, 38, 255}
    );
    DrawCircle(1010, 105, 46, {224, 231, 225, 235});
    DrawCircle(1025, 92, 42, {18, 23, 31, 255});

    for (int i = -5; i <= 14; ++i) {
        const float x = cameraX * 0.78f + i * 265.0f;
        const float h = 150.0f + static_cast<float>((i * 37 + 800) % 180);
        DrawFallbackBuilding(x, 420.0f, 220.0f, h, i + 20);
    }

    DrawRectangleGradientV(
        static_cast<int>(left), 432, static_cast<int>(width), 288,
        {38, 40, 43, 255}, {14, 16, 18, 255}
    );
    DrawPlayLayer(cameraX);
}

void Scene::DrawForeground(float cameraX) const {
    const float left = cameraX - 900.0f;
    const float width = 3080.0f;

    // Cinematic edge shading without darkening the combat lane excessively.
    DrawRectangleGradientV(
        static_cast<int>(left), 0, static_cast<int>(width), 78,
        {0, 0, 0, 135}, {0, 0, 0, 0}
    );
    DrawRectangleGradientV(
        static_cast<int>(left), 655, static_cast<int>(width), 65,
        {0, 0, 0, 25}, {0, 0, 0, 185}
    );

    for (int i = -3; i <= 10; ++i) {
        const float x = TileWorldX(cameraX, static_cast<float>(i), 1.04f) + 260.0f;
        DrawLine(
            static_cast<int>(x), 0,
            static_cast<int>(x + 220.0f), 142,
            {0, 0, 0, 34}
        );
    }
}

}
