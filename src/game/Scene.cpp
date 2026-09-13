#include "game/Scene.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {

float ParallaxX(float cameraX, float anchorX, float factor) {
    return cameraX * factor + anchorX;
}

Color BuildingColor(int index, bool mid) {
    const int base = mid ? 28 + (index % 3) * 7 : 18 + (index % 4) * 5;
    return {static_cast<unsigned char>(base), static_cast<unsigned char>(base + 5), static_cast<unsigned char>(base + 10), 255};
}

void DrawBuilding(float x, float y, float width, float height, int index, bool mid) {
    DrawRectangle(static_cast<int>(x), static_cast<int>(y - height), static_cast<int>(width), static_cast<int>(height), BuildingColor(index, mid));
    const int rows = std::max(2, static_cast<int>(height / 38.0f));
    const int cols = std::max(2, static_cast<int>(width / 34.0f));
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if ((r + c + index) % 4 == 0) continue;
            const int wx = static_cast<int>(x + 10.0f + c * 34.0f);
            const int wy = static_cast<int>(y - height + 18.0f + r * 38.0f);
            if (wx + 9 < x + width && wy + 13 < y) {
                const unsigned char glow = static_cast<unsigned char>(mid ? 125 : 85);
                DrawRectangle(wx, wy, 9, 13, {glow, static_cast<unsigned char>(glow + 25), 55, 130});
            }
        }
    }
}

void DrawLamp(float x, float baseY, float scale) {
    const int ix = static_cast<int>(x);
    DrawRectangle(ix - static_cast<int>(2 * scale), static_cast<int>(baseY - 132 * scale),
                  static_cast<int>(4 * scale), static_cast<int>(132 * scale), {18, 21, 23, 255});
    DrawLine(ix - static_cast<int>(4 * scale), static_cast<int>(baseY - 132 * scale),
             ix + static_cast<int>(18 * scale), static_cast<int>(baseY - 145 * scale), {18, 21, 23, 255});
    DrawCircle(ix + static_cast<int>(19 * scale), static_cast<int>(baseY - 146 * scale),
               std::max(2.0f, 5.0f * scale), {255, 205, 110, 180});
    DrawCircleGradient(ix + static_cast<int>(19 * scale), static_cast<int>(baseY - 146 * scale),
                       24.0f * scale, {255, 190, 90, 35}, {255, 190, 90, 0});
}

void DrawCar(float x, float y, float scale, bool faceRight) {
    const int ix = static_cast<int>(x);
    const int iy = static_cast<int>(y);
    const int w = static_cast<int>(130 * scale);
    const int h = static_cast<int>(30 * scale);
    DrawRectangle(ix, iy - h, w, h, {21, 25, 28, 255});
    DrawRectangle(ix + static_cast<int>(18 * scale), iy - h - static_cast<int>(18 * scale),
                  static_cast<int>(55 * scale), static_cast<int>(18 * scale), {25, 29, 32, 255});
    DrawRectangle(ix + static_cast<int>(25 * scale), iy - h - static_cast<int>(13 * scale),
                  static_cast<int>(20 * scale), static_cast<int>(9 * scale), {65, 88, 100, 180});
    DrawRectangle(ix + static_cast<int>(48 * scale), iy - h - static_cast<int>(13 * scale),
                  static_cast<int>(20 * scale), static_cast<int>(9 * scale), {65, 88, 100, 180});
    DrawCircle(ix + static_cast<int>(22 * scale), iy, static_cast<int>(10 * scale), {8, 10, 12, 255});
    DrawCircle(ix + static_cast<int>(w - 22 * scale), iy, static_cast<int>(10 * scale), {8, 10, 12, 255});
    if (faceRight) DrawRectangle(ix + w - static_cast<int>(4 * scale), iy - static_cast<int>(17 * scale), static_cast<int>(5 * scale), static_cast<int>(7 * scale), {255, 90, 75, 200});
}

void DrawTrashCluster(float x, float y) {
    DrawRectangle(static_cast<int>(x), static_cast<int>(y - 36), 28, 36, {28, 30, 31, 255});
    DrawRectangle(static_cast<int>(x + 4), static_cast<int>(y - 40), 20, 5, {45, 48, 48, 255});
    DrawCircle(static_cast<int>(x + 36), static_cast<int>(y - 10), 13, {16, 18, 18, 255});
    DrawCircle(static_cast<int>(x + 47), static_cast<int>(y - 9), 9, {22, 23, 23, 255});
}

}

void Scene::Init() {
}

void Scene::DrawBackground(float cameraX) const {
    const float left = cameraX - 900.0f;
    const float width = 3080.0f;

    // Night sky and atmospheric gradient.
    DrawRectangleGradientV(static_cast<int>(left), 0, static_cast<int>(width), 470,
                           {7, 12, 18, 255}, {26, 29, 38, 255});

    const float moonX = ParallaxX(cameraX, 1010.0f, 0.15f);
    DrawCircle(static_cast<int>(moonX), 105, 46, {224, 231, 225, 235});
    DrawCircle(static_cast<int>(moonX + 15), 92, 42, {18, 23, 31, 255});

    // Distant skyline: slower than the gameplay plane, giving the street actual depth.
    for (int i = -5; i <= 14; ++i) {
        const float x = ParallaxX(cameraX, i * 265.0f, 0.22f);
        const float h = 150.0f + static_cast<float>((i * 37 + 800) % 180);
        DrawBuilding(x, 420.0f, 220.0f, h, i + 20, false);
    }

    // Midground industrial-urban frontage.
    for (int i = -5; i <= 11; ++i) {
        const float x = ParallaxX(cameraX, i * 330.0f, 0.48f);
        const float h = 130.0f + static_cast<float>((i * 61 + 600) % 150);
        DrawBuilding(x, 448.0f, 280.0f, h, i + 50, true);
    }

    // Distant haze and power lines.
    DrawRectangleGradientH(static_cast<int>(left), 355, static_cast<int>(width), 110,
                           {60, 110, 125, 14}, {20, 35, 40, 0});
    for (int i = -4; i <= 14; ++i) {
        const float x = ParallaxX(cameraX, i * 430.0f, 0.48f);
        DrawLine(static_cast<int>(x), 335, static_cast<int>(x + 300), 375, {8, 12, 15, 120});
    }

    // Gameplay street plane, moving 1:1 with the camera.
    DrawRectangleGradientV(static_cast<int>(left), 432, static_cast<int>(width), 288,
                           {38, 40, 43, 255}, {14, 16, 18, 255});
    DrawRectangle(static_cast<int>(left), 454, static_cast<int>(width), 30, {57, 58, 59, 255});
    DrawRectangle(static_cast<int>(left), 482, static_cast<int>(width), 6, {17, 19, 21, 255});

    // Sidewalk slabs.
    for (int i = -5; i <= 12; ++i) {
        const int x = static_cast<int>(std::floor(cameraX / 180.0f) * 180.0f + i * 180.0f);
        DrawLine(x, 455, x, 482, {82, 84, 84, 95});
    }

    // Subtle lane guides used for depth movement, deliberately softer than a grid.
    DrawLine(static_cast<int>(left), 518, static_cast<int>(left + width), 518, {96, 98, 100, 32});
    DrawLine(static_cast<int>(left), 585, static_cast<int>(left + width), 585, {96, 98, 100, 28});
    DrawLine(static_cast<int>(left), 642, static_cast<int>(left + width), 642, {96, 98, 100, 22});

    // Crosswalks and street markings.
    for (int block = -2; block <= 14; ++block) {
        const int baseX = static_cast<int>(std::floor(cameraX / 1050.0f) * 1050.0f + block * 1050.0f);
        for (int stripe = 0; stripe < 6; ++stripe) {
            DrawRectangle(baseX + stripe * 26, 540, 16, 3, {170, 171, 168, 75});
        }
    }

    // Repeatable street props.
    for (int i = -4; i <= 13; ++i) {
        const float x = ParallaxX(cameraX, i * 650.0f + 120.0f, 1.0f);
        DrawLamp(x, 458.0f, 0.86f);
        if (i % 3 == 0) DrawCar(x - 190.0f, 487.0f, 1.0f, i % 2 == 0);
        if (i % 4 == 0) DrawTrashCluster(x + 95.0f, 518.0f);
    }

    // A few hard-edged industrial details connect the street to the Steel Yard identity.
    for (int i = -3; i <= 10; ++i) {
        const float x = ParallaxX(cameraX, i * 800.0f + 420.0f, 0.66f);
        DrawRectangle(static_cast<int>(x), 380, 82, 70, {15, 19, 21, 210});
        DrawRectangle(static_cast<int>(x + 10), 390, 62, 9, {65, 73, 76, 130});
        DrawRectangle(static_cast<int>(x + 10), 412, 62, 9, {65, 73, 76, 100});
        DrawLine(static_cast<int>(x + 12), 448, static_cast<int>(x + 70), 380, {90, 48, 35, 90});
    }
}

void Scene::DrawForeground(float cameraX) const {
    const float left = cameraX - 900.0f;
    const float width = 3080.0f;

    // Close foreground silhouettes create depth without obscuring the combat plane.
    DrawRectangleGradientV(static_cast<int>(left), 660, static_cast<int>(width), 60,
                           {0, 0, 0, 30}, {0, 0, 0, 190});

    for (int i = -3; i <= 10; ++i) {
        const float x = ParallaxX(cameraX, i * 900.0f + 250.0f, 1.04f);
        DrawLine(static_cast<int>(x), 0, static_cast<int>(x + 240), 155, {0, 0, 0, 26});
    }

    DrawRectangleGradientV(static_cast<int>(left), 0, static_cast<int>(width), 74,
                           {0, 0, 0, 155}, {0, 0, 0, 0});
}

}
