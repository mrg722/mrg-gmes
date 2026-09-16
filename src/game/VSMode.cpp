#include "game/VSMode.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace district_fury {
namespace {
constexpr float kMinX = 180.0f;
constexpr float kMaxX = 1120.0f;
constexpr float kMinY = 505.0f;
constexpr float kMaxY = 625.0f;
constexpr int kFieldCount = 7;

const char* kStageNames[] = {
    "STAGE 1 // SLUM DISTRICT",
    "STAGE 2 // OLD STEEL YARD",
    "STAGE 3 // ASTRA TOWER"
};

const int kScenarioCounts[] = {4, 5, 3};

const char* kStage1Scenarios[] = {
    "BARRIO BAJO // BLOQUE 17",
    "MERCADO ANTIGUO // LINEA DEL CANAL",
    "PUERTA DE ACERO // RUTA DE CARGA",
    "ASTILLERO DE CADENAS // TERRITORIO DE BRAKK"
};

const char* kEnemyNames[] = {
    "PUNK",
    "BRUTE",
    "CHARGER",
    "ENFORCER",
    "CHEMICAL SOLDIER",
    "URBAN NINJA",
    "MUTANT",
    "ARMORED GUARD"
};

StreetEnemyType NextEnemyType(StreetEnemyType type, int direction) {
    int index = static_cast<int>(type);
    index = (index + direction + 8) % 8;
    return static_cast<StreetEnemyType>(index);
}
}

VSMode::VSMode() = default;

void VSMode::Init() {
    flow = VSFlow::Select;
    stage = 0;
    scenario = 0;
    enemyCount = 1;
    cursor = 0;
    exitRequested = false;
    playerDefeated = false;
    enemyTypes = {StreetEnemyType::Punk, StreetEnemyType::Brute, StreetEnemyType::Charger, StreetEnemyType::Enforcer};
    enemies.clear();
    player.Reset();
    player.position = {360.0f, 585.0f, 0.0f};
}

bool VSMode::ShouldExit() const { return exitRequested; }

void VSMode::ClearExit() { exitRequested = false; }

int VSMode::ScenarioCount() const { return kScenarioCounts[std::clamp(stage, 0, 2)]; }

const char* VSMode::StageName() const { return kStageNames[std::clamp(stage, 0, 2)]; }

const char* VSMode::ScenarioText() const {
    if (stage == 0) return kStage1Scenarios[std::clamp(scenario, 0, 3)];
    return TextFormat("ESCENARIO %d/%d", scenario + 1, ScenarioCount());
}

const char* VSMode::BackgroundKey() const {
    return stage == 0 ? "bg_industrial" : "bg_steel_deep";
}

const char* VSMode::EnemyTypeName(StreetEnemyType type) {
    return kEnemyNames[static_cast<int>(type)];
}

void VSMode::ResetFight() {
    player.Reset();
    player.position = {360.0f, 585.0f, 0.0f};
    playerDefeated = false;
    enemies.clear();

    const std::array<float, 4> xs{700.0f, 835.0f, 970.0f, 1105.0f};
    const std::array<float, 4> ys{575.0f, 535.0f, 610.0f, 555.0f};
    for (int i = 0; i < enemyCount; ++i) {
        StreetEnemy enemy;
        enemy.Init({xs[static_cast<std::size_t>(i)], ys[static_cast<std::size_t>(i)], 0.0f}, enemyTypes[static_cast<std::size_t>(i)]);
        enemy.active = true;
        enemies.push_back(enemy);
    }
}

void VSMode::StartFight() {
    flow = VSFlow::Fight;
    ResetFight();
}

void VSMode::Update(float dt) {
    dt = std::min(dt, 0.033f);
    if (flow == VSFlow::Select) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            exitRequested = true;
            return;
        }
        if (IsKeyPressed(KEY_UP)) cursor = (cursor + kFieldCount - 1) % kFieldCount;
        if (IsKeyPressed(KEY_DOWN)) cursor = (cursor + 1) % kFieldCount;

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT)) {
            const int direction = IsKeyPressed(KEY_RIGHT) ? 1 : -1;
            if (cursor == 0) {
                stage = (stage + direction + 3) % 3;
                scenario = std::min(scenario, ScenarioCount() - 1);
            } else if (cursor == 1) {
                scenario = std::clamp(scenario + direction, 0, ScenarioCount() - 1);
            } else if (cursor == 2) {
                enemyCount = std::clamp(enemyCount + direction, 1, 4);
            } else {
                const int slot = cursor - 3;
                enemyTypes[static_cast<std::size_t>(slot)] = NextEnemyType(enemyTypes[static_cast<std::size_t>(slot)], direction);
            }
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_J)) StartFight();
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        flow = VSFlow::Select;
        enemies.clear();
        return;
    }
    if (IsKeyPressed(KEY_R)) {
        ResetFight();
        return;
    }
    if (playerDefeated) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_J)) ResetFight();
        return;
    }

    player.Update(dt);
    player.position.x = std::clamp(player.position.x, kMinX, kMaxX);
    player.position.y = std::clamp(player.position.y, kMinY, kMaxY);

    for (auto& enemy : enemies) {
        if (enemy.active && !enemy.IsDefeated()) {
            enemy.Update(dt, player);
            enemy.position.x = std::clamp(enemy.position.x, kMinX, kMaxX);
            enemy.position.y = std::clamp(enemy.position.y, kMinY, kMaxY);
        }
    }

    if (player.AttackIsActive() && !player.hasHit && player.attackType != AttackType::Energy) {
        const CombatBox hit = player.GetAttackHitbox();
        for (auto& enemy : enemies) {
            if (!enemy.active || enemy.IsDefeated() || !hit.Intersects(enemy.GetHurtbox())) continue;
            const float direction = player.facing == Facing::Right ? 1.0f : -1.0f;
            enemy.TakeDamage(player.GetAttackDamage() + (player.isRageMode ? 5 : 0), {direction * player.GetAttackKnockback(), 0.0f, 0.0f});
            player.hasHit = true;
            break;
        }
    }

    if (player.state != PlayerState::Defeat) {
        for (auto& enemy : enemies) {
            if (!enemy.active || enemy.IsDefeated() || enemy.hasHit || !enemy.AttackIsActive()) continue;
            if (enemy.GetAttackHitbox().Intersects(player.GetHurtbox())) {
                player.TakeDamage(static_cast<int>(enemy.attackDamage));
                enemy.hasHit = true;
                break;
            }
        }
    }

    playerDefeated = player.state == PlayerState::Defeat;
}

void VSMode::DrawBackground() const {
    DrawRectangle(0, 0, 1280, 720, {7, 11, 15, 255});
    if (stage < 2) {
        const Texture2D bg = AssetManager::Get().GetTexture(BackgroundKey());
        if (bg.id != 0) DrawTexturePro(bg, {0, 0, static_cast<float>(bg.width), static_cast<float>(bg.height)}, {0, 0, 1280, 720}, {0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangle(0, 135, 1280, 400, {10, 18, 38, 255});
        for (int x = 0; x < 1280; x += 280) {
            DrawRectangle(x, 170, 230, 240, {17, 27, 48, 255});
            DrawRectangle(x + 20, 195, 82, 70, {30, 75, 108, 255});
            DrawRectangle(x + 125, 195, 82, 70, {90, 42, 100, 255});
        }
        for (int x = 0; x < 1280; x += 160) {
            DrawLineEx({static_cast<float>(x), 360}, {static_cast<float>(x + 80), 430}, 4.0f, {35, 55, 85, 220});
            DrawLineEx({static_cast<float>(x + 80), 430}, {static_cast<float>(x + 160), 360}, 4.0f, {35, 55, 85, 220});
        }
    }
    DrawRectangle(0, 625, 1280, 95, {8, 11, 14, 255});
    for (int x = 0; x < 1280; x += 150) {
        DrawRectangle(x, 617, 110, 9, {58, 63, 63, 255});
        DrawRectangle(x + 30, 645, 65, 5, {90, 75, 50, 210});
    }
}

void VSMode::DrawHud() const {
    DrawRectangle(16, 16, 410, 126, {4, 8, 11, 225});
    DrawText("RAYDEN CRUZ // VS TEST", 30, 27, 20, {190, 220, 245, 255});
    DrawText(TextFormat("VIDA %d/%d", player.hp, player.maxHp), 30, 58, 15, WHITE);
    DrawRectangle(122, 61, 230, 12, {25, 25, 30, 255});
    DrawRectangle(122, 61, static_cast<int>(230.0f * player.hp / std::max(1, player.maxHp)), 12, {45, 170, 255, 255});
    DrawText(TextFormat("SP %d/%d", player.sp, player.maxSp), 30, 82, 14, {120, 215, 255, 255});
    DrawText(TextFormat("RAGE %d%%", player.rage), 180, 82, 14, player.isRageMode ? Color{255, 85, 70, 255} : Color{255, 210, 90, 255});
    DrawText(TextFormat("STAGE %d  //  %s", stage + 1, ScenarioText()), 30, 106, 13, {180, 205, 215, 235});

    DrawRectangle(850, 16, 414, 126, {4, 8, 11, 215});
    DrawText(StageName(), 875, 27, 17, {255, 205, 85, 255});
    DrawText(TextFormat("ENEMIGOS: %d", enemyCount), 875, 54, 14, WHITE);
    for (int i = 0; i < enemyCount; ++i) {
        const auto& enemy = enemies[static_cast<std::size_t>(i)];
        DrawText(TextFormat("%d  %s", i + 1, EnemyTypeName(enemy.type)), 875, 76 + i * 17, 12, enemy.IsDefeated() ? Color{120, 130, 135, 180} : WHITE);
    }
}

void VSMode::DrawSelection() const {
    DrawBackground();
    DrawRectangle(230, 65, 820, 560, {4, 8, 12, 238});
    DrawText("MODO VS / PRUEBA", 445, 91, 42, {225, 235, 240, 255});
    DrawText("Selecciona una prueba antes de entrar directamente a la arena.", 355, 138, 16, {155, 180, 190, 230});

    const int y[kFieldCount] = {190, 235, 280, 325, 370, 415, 460};
    for (int i = 0; i < kFieldCount; ++i) {
        const bool selected = cursor == i;
        DrawRectangleLines(350, y[i] - 7, 580, 38, selected ? Color{255, 205, 75, 170} : Color{100, 120, 130, 90});
    }

    const Color active = {255, 220, 90, 255};
    DrawText("STAGE", 375, y[0], 17, cursor == 0 ? active : WHITE);
    DrawText(StageName(), 570, y[0], 17, {190, 215, 225, 255});

    DrawText("ESCENARIO", 375, y[1], 17, cursor == 1 ? active : WHITE);
    DrawText(ScenarioText(), 570, y[1], 17, {190, 215, 225, 255});

    DrawText("CANTIDAD", 375, y[2], 17, cursor == 2 ? active : WHITE);
    DrawText(TextFormat("%d ENEMIGO%s", enemyCount, enemyCount == 1 ? "" : "S"), 570, y[2], 17, {190, 215, 225, 255});

    for (int i = 0; i < 4; ++i) {
        const int row = 3 + i;
        const bool enabled = i < enemyCount;
        DrawText(TextFormat("ENEMIGO %d", i + 1), 375, y[row], 17, cursor == row ? active : (enabled ? WHITE : Color{100, 110, 115, 150}));
        DrawText(EnemyTypeName(enemyTypes[static_cast<std::size_t>(i)]), 570, y[row], 17, enabled ? Color{190, 215, 225, 255} : Color{100, 110, 115, 150});
    }

    DrawText("↑/↓ CAMPO   ←/→ VALOR   ENTER/J COMENZAR", 390, 535, 14, {155, 180, 190, 235});
    DrawText("ESC VOLVER AL MENU PRINCIPAL", 475, 565, 14, {135, 155, 165, 210});
}

void VSMode::DrawFight() const {
    DrawBackground();
    std::array<std::pair<float, int>, 5> drawOrder{};
    int count = 0;
    drawOrder[count++] = {player.position.y, -1};
    for (int i = 0; i < static_cast<int>(enemies.size()); ++i) {
        if (enemies[static_cast<std::size_t>(i)].active) drawOrder[count++] = {enemies[static_cast<std::size_t>(i)].position.y, i};
    }
    std::sort(drawOrder.begin(), drawOrder.begin() + count, [](const auto& a, const auto& b) { return a.first < b.first; });
    for (int i = 0; i < count; ++i) {
        if (drawOrder[i].second < 0) player.Draw();
        else enemies[static_cast<std::size_t>(drawOrder[i].second)].Draw();
    }

    for (int i = 0; i < static_cast<int>(enemies.size()); ++i) {
        const auto& enemy = enemies[static_cast<std::size_t>(i)];
        const Vector2 p = enemy.position.ToScreen();
        const int width = 90;
        const int x = static_cast<int>(p.x) - width / 2;
        const int y = static_cast<int>(p.y) - 118;
        DrawRectangle(x, y, width, 7, {20, 22, 25, 220});
        if (!enemy.IsDefeated()) DrawRectangle(x + 1, y + 1, static_cast<int>((width - 2) * enemy.hp / static_cast<float>(std::max(1, enemy.maxHp))), 5, {225, 65, 65, 255});
        DrawText(TextFormat("%d %s", i + 1, EnemyTypeName(enemy.type)), x, y - 17, 10, enemy.IsDefeated() ? Color{120, 130, 135, 190} : WHITE);
    }

    DrawHud();
    DrawRectangle(300, 660, 680, 42, {4, 8, 12, 220});
    DrawText("J GOLPE   K PATADA   L ENERGIA   B BLOQUEO   SHIFT DASH   SPACE FURIA", 335, 672, 12, {175, 200, 210, 235});
    DrawText("R REINICIAR   ESC CONFIGURACION", 495, 690, 11, {135, 155, 165, 215});

    if (playerDefeated) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 160});
        DrawText("RAYDEN DERROTADO", 445, 290, 46, {240, 80, 80, 255});
        DrawText("ENTER/J REINICIAR   ESC CONFIGURACION", 430, 360, 19, WHITE);
    }
}

void VSMode::Draw() const {
    if (flow == VSFlow::Select) DrawSelection();
    else DrawFight();
}

}
