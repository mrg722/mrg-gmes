#include "raylib.h"
#include "audio/AudioSystem.h"
#include "game/Stage1StoryGame.h"
#include "game/Stage2Game.h"
#include "game/Stage3Game.h"
#include "game/VSMode.h"
#include "core/ApplicationState.h"
#include "rendering/AssetManager.h"

namespace {
void DrawMenuPrincipal() {
    DrawRectangle(0,0,1280,720,{4,7,11,255});
    Texture2D bg=district_fury::AssetManager::Get().GetTexture("bg_industrial");
    if(bg.id) DrawTexturePro(bg,{0,0,(float)bg.width,(float)bg.height},{0,0,1280,720},{0,0},0,{255,255,255,68});
    DrawRectangle(0,0,1280,720,{2,6,11,188});
    DrawRectangle(250,70,780,560,{4,9,14,238});
    DrawRectangleLines(250,70,780,560,{55,80,95,160});
    DrawRectangle(250,70,7,560,{60,205,240,230});
    DrawRectangle(1023,70,7,560,{255,205,75,210});
    DrawText("DISTRICT FURY",340,105,62,{225,235,240,255});
    DrawText("BEAT-EM-UP DE HISTORIA",445,172,20,{75,215,240,255});
    DrawText("CAPITULO 1 // LA RUTA DE LAS CADENAS",370,214,22,{255,205,75,255});
    DrawRectangle(410,280,460,58,{10,19,27,235});
    DrawRectangleLines(410,280,460,58,{255,205,75,190});
    DrawText("ENTER / J",440,300,18,{255,225,135,255});
    DrawText("NUEVA PARTIDA",595,300,20,WHITE);
    DrawRectangle(410,355,460,48,{8,16,23,220});
    DrawText("C",440,369,18,{80,215,240,255});
    DrawText("CONTROLES",595,369,18,{210,220,225,255});
    DrawRectangle(410,417,460,48,{8,16,23,220});
    DrawText("↑ / ↓",440,431,18,{255,205,75,255});
    DrawText("DIFICULTAD",595,431,18,{210,220,225,255});
    DrawRectangle(410,479,460,58,{8,16,23,235});
    DrawRectangleLines(410,479,460,58,{70,200,230,180});
    DrawText("V",440,498,20,{80,220,245,255});
    DrawText("MODO VS // LABORATORIO",595,498,20,{220,230,235,255});
    DrawText("ENTER/J  JUGAR     C  CONTROLES     ↑/↓  DIFICULTAD     V  MODO VS",342,585,13,{150,180,190,235});
    DrawText("El Modo VS permite probar stages, escenarios y hasta 4 enemigos.",350,608,12,{125,160,175,230});
    DrawText("PC // 60 FPS",965,682,11,{105,130,140,210});
}
}

int main(){
    constexpr int windowWidth=1280,windowHeight=720;
    InitWindow(windowWidth,windowHeight,"District Fury");
    SetExitKey(KEY_NULL); SetTargetFPS(60);
    district_fury::AssetManager::Get().LoadAll();
    district_fury::AudioSystem::Get().Init();
    district_fury::Stage1StoryGame stage1; district_fury::Stage2Game stage2; district_fury::Stage3Game stage3; district_fury::VSMode vsMode;
    stage1.Init(); stage2.Init(); stage3.Init(); vsMode.Init();
    int activeStage=1; bool vsActive=false;
    district_fury::core::ApplicationState state=district_fury::core::ApplicationState::Running;
    while(district_fury::core::shouldContinue(state)){
        if(WindowShouldClose()){state=district_fury::core::ApplicationState::ExitRequested;continue;}
        if(vsActive){vsMode.Update(GetFrameTime());if(vsMode.ShouldExit()){vsMode.ClearExit();vsActive=false;stage1.ReturnToMenu();}}
        else{
            if(stage1.IsMenu()&&IsKeyPressed(KEY_V)){vsMode.Init();vsActive=true;}
            if(!vsActive){
                if(IsKeyPressed(KEY_F1)){activeStage=1;stage1.Init();}
                if(IsKeyPressed(KEY_F2)){activeStage=2;stage2.Init();}
                if(IsKeyPressed(KEY_F3)){activeStage=3;stage3.Init();}
                if(activeStage==1)stage1.Update(GetFrameTime()); else if(activeStage==2)stage2.Update(GetFrameTime()); else stage3.Update(GetFrameTime());
            }
        }
        BeginDrawing(); ClearBackground({8,11,11,255});
        if(vsActive)vsMode.Draw();
        else if(activeStage==1){if(stage1.IsMenu())DrawMenuPrincipal();else stage1.Draw();}
        else if(activeStage==2)stage2.Draw();
        else stage3.Draw();
        if(!vsActive&&!stage1.IsMenu()){DrawRectangle(1010,684,254,24,{5,8,10,185});DrawText("F1 S1  |  F2 S2  |  F3 S3",1020,689,12,{180,200,205,210});}
        EndDrawing();
    }
    district_fury::AudioSystem::Get().Shutdown(); district_fury::AssetManager::Get().UnloadAll(); CloseWindow(); return 0;
}
