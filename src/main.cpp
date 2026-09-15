#include "raylib.h"
#include "game/ProductionGame.h"
#include "core/ApplicationState.h"
#include "rendering/AssetManager.h"

int main() {
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    InitWindow(windowWidth, windowHeight, "District Fury");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    district_fury::AssetManager::Get().LoadAll();

    district_fury::ProductionGame game;
    game.Init();

    district_fury::core::ApplicationState state = district_fury::core::ApplicationState::Running;
    while (district_fury::core::shouldContinue(state)) {
        if (WindowShouldClose()) {
            state = district_fury::core::ApplicationState::ExitRequested;
            continue;
        }

        game.Update(GetFrameTime());

        BeginDrawing();
        ClearBackground({8, 11, 11, 255});
        game.Draw();
        EndDrawing();
    }

    district_fury::AssetManager::Get().UnloadAll();
    CloseWindow();
    return 0;
}
