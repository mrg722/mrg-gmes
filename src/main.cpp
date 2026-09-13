#include "raylib.h"
#include "core/ApplicationState.h"
#include "game/GameManager.h"

int main() {
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    InitWindow(windowWidth, windowHeight, "District Fury");
    SetTargetFPS(60);

    district_fury::GameManager game;
    game.Init();

    district_fury::core::ApplicationState state = district_fury::core::ApplicationState::Running;
    while (district_fury::core::shouldContinue(state)) {
        if (WindowShouldClose()) {
            state = district_fury::core::ApplicationState::ExitRequested;
            continue;
        }

        float dt = GetFrameTime();
        game.Update(dt);

        BeginDrawing();
        game.Draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
