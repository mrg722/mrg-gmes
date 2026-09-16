#include "raylib.h"
#include "audio/AudioSystem.h"
#include "game/Stage1StoryGame.h"
#include "game/Stage2Game.h"
#include "game/Stage3Game.h"
#include "game/VSMode.h"
#include "core/ApplicationState.h"
#include "rendering/AssetManager.h"

namespace {
void DrawMenuEnhancement() {
    DrawRectangle(322, 272, 636, 290, {4, 8, 12, 165});
    DrawRectangleLines(322, 272, 636, 290, {70, 105, 125, 120});
    DrawRectangle(343, 294, 594, 1, {90, 145, 170, 95});
    DrawText("MODO HISTORIA", 368, 316, 18, {130, 205, 225, 220});
    DrawRectangle(368, 345, 260, 44, {255, 205, 80, 42});
    DrawRectangleLines(368, 345, 260, 44, {255, 205, 80, 150});
    DrawText("ENTER / J", 389, 357, 16, {255, 220, 110, 255});
    DrawText("NUEVA PARTIDA", 495, 357, 16, WHITE);
    DrawRectangleLines(368, 399, 260, 44, {95, 175, 205, 135});
    DrawText("V", 389, 411, 18, {255, 205, 85, 255});
    DrawText("MODO VS / PRUEBA", 495, 411, 16, {205, 225, 235, 255});
    DrawRectangleLines(368, 453, 260, 44, {95, 175, 205, 110});
    DrawText("C", 389, 465, 18, {120, 215, 240, 255});
    DrawText("CONTROLES", 495, 465, 16, {205, 225, 235, 255});
    DrawText("↑ / ↓", 666, 357, 15, {180, 200, 210, 235});
    DrawText("DIFICULTAD", 746, 357, 15, {180, 200, 210, 235});
    DrawText("F1  S1     F2  S2     F3  S3", 665, 414, 13, {155, 185, 200, 220});
    DrawText("ESC", 665, 468, 14, {155, 185, 200, 220});
    DrawText("salir del juego", 710, 468, 14, {155, 185, 200, 220});
    DrawRectangle(343, 520, 594, 1, {90, 145, 170, 80});
    DrawText("LABORATORIO DE COMBATE  //  SPRITES + HITBOX + ESCENARIOS", 366, 532, 12, {120, 155, 170, 210});
}
}

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
            if (stage1.IsMenu()) DrawMenuEnhancement();
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
