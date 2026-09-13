#include "game/Scene.h"

namespace district_fury {

void Scene::Init() {
}

void Scene::DrawBackground() const {
    // Old Steel Yard - Dark industrial
    ClearBackground({20, 25, 25, 255}); // dark greenish-grey

    // Distant background (pipes/machinery)
    DrawRectangle(0, 0, 1280, 400, {30, 35, 30, 255});
    
    // Floor
    DrawRectangle(0, 400, 1280, 320, {40, 45, 45, 255});

    // Floor lines for perspective / depth
    for (int i = 0; i < 5; i++) {
        DrawLine(0, 450 + i * 50, 1280, 450 + i * 50, {30, 35, 35, 255});
    }

    // Some background industrial props
    DrawRectangle(100, 200, 80, 200, {50, 55, 50, 255});
    DrawRectangle(800, 150, 120, 250, {45, 50, 45, 255});
    
    // Toxic green glow
    DrawCircleGradient(200, 400, 150, {0, 255, 0, 30}, {0, 255, 0, 0});
    DrawCircleGradient(900, 400, 200, {0, 255, 0, 20}, {0, 255, 0, 0});
}

void Scene::DrawForeground() const {
    // Vignette or foreground elements
    DrawRectangleGradientV(0, 0, 1280, 100, {0,0,0,100}, {0,0,0,0});
    DrawRectangleGradientV(0, 620, 1280, 100, {0,0,0,0}, {0,0,0,100});
}

}
