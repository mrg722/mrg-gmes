#include "raylib.h"
#include "audio/AudioSystem.h"
#include "game/ProductionGame.h"
#include "game/Stage2Game.h"
#include "core/ApplicationState.h"
#include "rendering/AssetManager.h"

int main() {
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;
    InitWindow(windowWidth, windowHeight, "District Fury");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    district_fury::AssetManager::Get().LoadAll();
    district_fury::AudioSystem::Get().Init();

    district_fury::ProductionGame stage1;
    district_fury::Stage2Game stage2;
    stage1.Init();
    stage2.Init();
    bool usingStage2 = false;

    district_fury::core::ApplicationState state = district_fury::core::ApplicationState::Running;
    while (district_fury::core::shouldContinue(state)) {
        if (WindowShouldClose()) {
            state = district_fury::core::ApplicationState::ExitRequested;
            continue;
        }

        if (IsKeyPressed(KEY_F2)) { usingStage2 = true; stage2.Init(); }
        if (IsKeyPressed(KEY_F3)) { usingStage2 = false; stage1.Init(); }

        if (usingStage2) stage2.Update(GetFrameTime());
        else stage1.Update(GetFrameTime());

        BeginDrawing();
        ClearBackground({8, 11, 11, 255});
        if (usingStage2) stage2.Draw();
        else stage1.Draw();
        DrawRectangle(1040, 684, 224, 24, {5, 8, 10, 185});
        DrawText("F2 STAGE 2  |  F3 STAGE 1", 1050, 689, 12, {180, 200, 205, 210});
        EndDrawing();
    }

    district_fury::AudioSystem::Get().Shutdown();
    district_fury::AssetManager::Get().UnloadAll();
    CloseWindow();
    return 0;
}
