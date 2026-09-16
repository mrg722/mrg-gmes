#include "raylib.h"
#include "audio/AudioSystem.h"
#include "game/Stage1StoryGame.h"
#include "game/Stage2Game.h"
#include "game/Stage3Game.h"
#include "game/VSMode.h"
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

    district_fury::Stage1StoryGame stage1;
    district_fury::Stage2Game stage2;
    district_fury::Stage3Game stage3;
    district_fury::VSMode vsMode;
    stage1.Init();
    stage2.Init();
    stage3.Init();
    vsMode.Init();
    int activeStage = 1;
    bool vsActive = false;

    district_fury::core::ApplicationState state = district_fury::core::ApplicationState::Running;
    while (district_fury::core::shouldContinue(state)) {
        if (WindowShouldClose()) {
            state = district_fury::core::ApplicationState::ExitRequested;
            continue;
        }

        if (vsActive) {
            vsMode.Update(GetFrameTime());
            if (vsMode.ShouldExit()) {
                vsMode.ClearExit();
                vsActive = false;
                stage1.ReturnToMenu();
            }
        } else {
            if (stage1.IsMenu() && IsKeyPressed(KEY_V)) {
                vsMode.Init();
                vsActive = true;
            }

            if (!vsActive) {
                if (IsKeyPressed(KEY_F1)) { activeStage = 1; stage1.Init(); }
                if (IsKeyPressed(KEY_F2)) { activeStage = 2; stage2.Init(); }
                if (IsKeyPressed(KEY_F3)) { activeStage = 3; stage3.Init(); }

                if (activeStage == 1) stage1.Update(GetFrameTime());
                else if (activeStage == 2) stage2.Update(GetFrameTime());
                else stage3.Update(GetFrameTime());
            }
        }

        BeginDrawing();
        ClearBackground({8, 11, 11, 255});
        if (vsActive) {
            vsMode.Draw();
        } else if (activeStage == 1) {
            stage1.Draw();
            if (stage1.IsMenu()) {
                DrawRectangle(470, 560, 340, 38, {4, 8, 12, 225});
                DrawText("V: MODO VS / PRUEBA", 505, 570, 17, {255, 205, 85, 255});
            }
        } else if (activeStage == 2) {
            stage2.Draw();
        } else {
            stage3.Draw();
        }
        if (!vsActive) {
            DrawRectangle(1010, 684, 254, 24, {5, 8, 10, 185});
            DrawText("F1 S1  |  F2 S2  |  F3 S3", 1020, 689, 12, {180, 200, 205, 210});
        }
        EndDrawing();
    }

    district_fury::AudioSystem::Get().Shutdown();
    district_fury::AssetManager::Get().UnloadAll();
    CloseWindow();
    return 0;
}
