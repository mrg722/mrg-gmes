#include "raylib.h"

#include "core/ApplicationState.h"

int main() {
    constexpr int windowWidth = 1280;
    constexpr int windowHeight = 720;

    InitWindow(windowWidth, windowHeight, "District Fury");
    SetTargetFPS(60);

    district_fury::core::ApplicationState state = district_fury::core::ApplicationState::Running;
    while (district_fury::core::shouldContinue(state)) {
        if (WindowShouldClose()) {
            state = district_fury::core::ApplicationState::ExitRequested;
            continue;
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawText("District Fury", 48, 48, 32, RAYWHITE);
        DrawText("Technical bootstrap", 48, 92, 20, LIGHTGRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
