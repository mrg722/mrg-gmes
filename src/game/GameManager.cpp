#include "game/GameManager.h"
#include "raylib.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace district_fury {
namespace {

struct SpawnDefinition {
    float x;
    float y;
    StreetEnemyType type;
};

constexpr SpawnDefinition kStageSpawns[] = {
    {900.0f, 565.0f, StreetEnemyType::Punk},
    {1130.0f, 520.0f, StreetEnemyType::Charger},
    {1470.0f, 600.0f, StreetEnemyType::Brute},
    {1730.0f, 545.0f, StreetEnemyType::Punk},
    {2250.0f, 585.0f, StreetEnemyType::Punk},
    {2480.0f, 520.0f, StreetEnemyType::Charger},
    {2750.0f, 610.0f, StreetEnemyType::Brute},
    {3320.0f, 550.0f, StreetEnemyType::Charger},
    {3570.0f, 625.0f, StreetEnemyType::Punk},
    {3880.0f, 520.0f, StreetEnemyType::Brute},
    {4380.0f, 585.0f, StreetEnemyType::Punk},
    {4650.0f, 525.0f, StreetEnemyType::Charger},
    {5050.0f, 610.0f, StreetEnemyType::Brute},
    {5480.0f, 555.0f, StreetEnemyType::Enforcer}
};

}

GameManager::GameManager()
    : flowState(GameFlowState::Start),
      hitstopTimer(0.0f),
      screenshakeTimer(0.0f),
      introTimer(2.7f),
      elapsedTime(0.0f),
      cameraX(kViewportWidth * 0.5f),
      maxCombo(0),
      defeatedEnemies(0) {}

void GameManager::Init() {
    scene.Init();
    player.Reset();
    enemies.clear();

    for (const auto& spawn : kStageSpawns) {
        StreetEnemy enemy;
        enemy.Init({spawn.x, spawn.y, 0.0f}, spawn.type);
        enemies.push_back(enemy);
    }

    flowState = GameFlowState::Start;
    introTimer = 2.7f;
    hitstopTimer = 0.0f;
    screenshakeTimer = 0.0f;
    elapsedTime = 0.0f;
    cameraX = kViewportWidth * 0.5f;
    maxCombo = 0;
    defeatedEnemies = 0;
    damageTexts.clear();
    particles.clear();
}

void GameManager::SpawnDamageText(Vector3D pos, int damage, bool critical) {
    damageTexts.push_back({pos, damage, 0.65f, 0.65f, critical});
}

void GameManager::SpawnHitParticles(Vector3D pos, AttackType type, bool heavy) {
    const int count = heavy ? 22 : 12;
    const Color color =
        type == AttackType::Energy ? Color{45, 180, 255, 255} :
        type == AttackType::Kick ? Color{255, 170, 55, 255} :
        Color{255, 235, 150, 255};

    for (int i = 0; i < count; ++i) {
        const float angle = static_cast<float>(i) / static_cast<float>(count) * 6.2831853f;
        const float speed = static_cast<float>(GetRandomValue(90, heavy ? 340 : 240));
        Particle p;
        p.position = pos;
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.timer = heavy ? 0.48f : 0.30f;
        p.maxTime = p.timer;
        p.color = color;
        p.size = static_cast<float>(GetRandomValue(3, heavy ? 8 : 6));
        particles.push_back(p);
    }

    if (heavy) {
        for (int i = 0; i < 5; ++i) {
            Particle arc;
            arc.position = pos;
            arc.velocity = {
                static_cast<float>(GetRandomValue(-230, 230)),
                static_cast<float>(GetRandomValue(-140, 140))
            };
            arc.timer = 0.22f + i * 0.025f;
            arc.maxTime = arc.timer;
            arc.color = type == AttackType::Energy
                ? Color{0, 210, 255, 255}
                : Color{255, 120, 45, 255};
            arc.size = static_cast<float>(GetRandomValue(7, 14));
            particles.push_back(arc);
        }
    }
}

void GameManager::DoHitstop(float duration) {
    hitstopTimer = std::max(hitstopTimer, duration);
}

void GameManager::DoScreenshake(float duration) {
    screenshakeTimer = std::max(screenshakeTimer, duration);
}

int GameManager::RemainingEnemies() const {
    return static_cast<int>(std::count_if(
        enemies.begin(), enemies.end(),
        [](const StreetEnemy& enemy) { return !enemy.IsDefeated(); }
    ));
}

bool GameManager::AllEnemiesDefeated() const {
    return std::all_of(
        enemies.begin(), enemies.end(),
        [](const StreetEnemy& enemy) { return enemy.IsDefeated(); }
    );
}

void GameManager::Update(float dt) {
    if (flowState == GameFlowState::Start) {
        introTimer -= dt;
        if (introTimer <= 0.0f) flowState = GameFlowState::Combat;
        return;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
        if (flowState == GameFlowState::Combat) flowState = GameFlowState::Pause;
        else if (flowState == GameFlowState::Pause) flowState = GameFlowState::Combat;
    }

    if (flowState == GameFlowState::Pause) return;

    if (flowState == GameFlowState::Win || flowState == GameFlowState::GameOver) {
        if (IsKeyPressed(KEY_R)) Init();
        return;
    }

    if (hitstopTimer > 0.0f) {
        hitstopTimer = std::max(0.0f, hitstopTimer - dt);
        return;
    }

    screenshakeTimer = std::max(0.0f, screenshakeTimer - dt);
    elapsedTime += dt;

    for (auto& txt : damageTexts) txt.timer -= dt;
    damageTexts.erase(
        std::remove_if(
            damageTexts.begin(), damageTexts.end(),
            [](const DamageText& t) { return t.timer <= 0.0f; }
        ),
        damageTexts.end()
    );

    for (auto& p : particles) {
        p.timer -= dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.velocity.x *= 0.93f;
        p.velocity.y *= 0.93f;
    }

    particles.erase(
        std::remove_if(
            particles.begin(), particles.end(),
            [](const Particle& p) { return p.timer <= 0.0f; }
        ),
        particles.end()
    );

    player.Update(dt);

    // Stream encounters in as the camera approaches them.
    for (auto& enemy : enemies) {
        if (!enemy.active && player.position.x + 720.0f >= enemy.position.x) {
            enemy.Activate();
        }
    }

    for (auto& enemy : enemies) enemy.Update(dt, player);

    if (player.AttackIsActive() && !player.hasHit) {
        const CombatBox attackBox = player.GetAttackHitbox();
        int bestIndex = -1;
        float bestDistance = 1000000.0f;

        for (std::size_t i = 0; i < enemies.size(); ++i) {
            StreetEnemy& enemy = enemies[i];
            if (!enemy.active || enemy.IsDefeated()) continue;
            if (!attackBox.Intersects(enemy.GetHurtbox())) continue;

            const float dx = enemy.position.x - player.position.x;
            const float dy = enemy.position.y - player.position.y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance < bestDistance) {
                bestDistance = distance;
                bestIndex = static_cast<int>(i);
            }
        }

        if (bestIndex >= 0) {
            StreetEnemy& enemy = enemies[static_cast<std::size_t>(bestIndex)];
            const int damage = player.GetAttackDamage();
            const bool critical =
                player.attackType == AttackType::Kick ||
                (player.comboStep >= 2 && player.attackType == AttackType::Punch);
            const float direction = player.facing == Facing::Right ? 1.0f : -1.0f;

            enemy.TakeDamage(
                damage,
                {direction * player.GetAttackKnockback(), 0.0f, 0.0f}
            );

            player.hasHit = true;
            player.comboCount++;
            player.comboWindow = 0.9f;
            maxCombo = std::max(maxCombo, player.comboCount);

            SpawnDamageText(enemy.position, damage, critical);
            SpawnHitParticles(
                {enemy.position.x, enemy.position.y - 58.0f, enemy.position.z},
                player.attackType,
                critical
            );

            DoHitstop(player.attackType == AttackType::Energy ? 0.12f : 0.075f);
            DoScreenshake(critical ? 0.15f : 0.09f);
        }
    }

    if (player.state != PlayerState::Defeat) {
        for (auto& enemy : enemies) {
            if (!enemy.active || enemy.IsDefeated() || enemy.hasHit) continue;
            if (!enemy.AttackIsActive()) continue;
            if (!enemy.GetAttackHitbox().Intersects(player.GetHurtbox())) continue;

            const int damage = static_cast<int>(enemy.attackDamage);
            player.TakeDamage(damage);
            enemy.hasHit = true;
            player.velocity = {
                (enemy.facing == Facing::Right ? 1.0f : -1.0f) * 240.0f,
                0.0f,
                0.0f
            };
            player.comboCount = 0;
            player.comboWindow = 0.0f;

            SpawnDamageText(player.position, damage, false);
            SpawnHitParticles(
                {player.position.x, player.position.y - 48.0f, 0.0f},
                AttackType::Punch,
                false
            );
            DoHitstop(0.08f);
            DoScreenshake(0.12f);
            break;
        }
    }

    defeatedEnemies = static_cast<int>(std::count_if(
        enemies.begin(), enemies.end(),
        [](const StreetEnemy& enemy) { return enemy.IsDefeated(); }
    ));

    if (player.position.x >= kStageEndX - 220.0f && AllEnemiesDefeated()) {
        flowState = GameFlowState::Win;
    } else if (player.state == PlayerState::Defeat) {
        flowState = GameFlowState::GameOver;
    }

    const float cameraMin = kViewportWidth * 0.5f;
    const float cameraMax = kStageEndX - kViewportWidth * 0.5f;
    const float desiredCamera = std::clamp(player.position.x, cameraMin, cameraMax);
    const float cameraEase = 1.0f - std::pow(0.001f, std::max(0.0f, dt));
    cameraX += (desiredCamera - cameraX) * cameraEase;
}

void GameManager::Draw() const {
    Camera2D camera{};
    camera.offset = {kViewportWidth * 0.5f, kViewportHeight * 0.5f};
    camera.target = {cameraX, kViewportHeight * 0.5f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    if (screenshakeTimer > 0.0f) {
        const float strength = 4.5f;
        camera.target.x += static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * strength;
        camera.target.y += static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * strength;
    }

    BeginMode2D(camera);
    scene.DrawBackground(cameraX);

    struct DrawItem { float depth; int index; bool playerItem; };
    std::vector<DrawItem> drawOrder;
    drawOrder.reserve(enemies.size() + 1);
    drawOrder.push_back({player.position.y, -1, true});

    for (std::size_t i = 0; i < enemies.size(); ++i) {
        if (enemies[i].active) {
            drawOrder.push_back({enemies[i].position.y, static_cast<int>(i), false});
        }
    }

    std::sort(
        drawOrder.begin(), drawOrder.end(),
        [](const DrawItem& a, const DrawItem& b) { return a.depth < b.depth; }
    );

    for (const auto& item : drawOrder) {
        if (item.playerItem) player.Draw();
        else enemies[static_cast<std::size_t>(item.index)].Draw();
    }

    for (const auto& p : particles) {
        const Vector2 pos = p.position.ToScreen();
        const float alpha = std::clamp(p.timer / p.maxTime, 0.0f, 1.0f);
        Color c = p.color;
        c.a = static_cast<unsigned char>(255.0f * alpha);
        DrawCircle(
            static_cast<int>(pos.x),
            static_cast<int>(pos.y),
            p.size * alpha + 1.0f,
            c
        );
    }

    for (const auto& txt : damageTexts) {
        const Vector2 pos = txt.position.ToScreen();
        const float progress = 1.0f - (txt.timer / txt.maxTime);
        const int size = txt.critical ? 34 : 28;
        const Color color = txt.critical ? Color{255, 195, 70, 255} : WHITE;
        DrawText(
            TextFormat("%s%d", txt.critical ? "CRIT " : "", txt.amount),
            static_cast<int>(pos.x - 25.0f),
            static_cast<int>(pos.y - 105.0f - progress * 55.0f),
            size,
            color
        );
    }

    scene.DrawForeground(cameraX);
    EndMode2D();

    // HUD composition: original arcade layout, tuned to keep the action readable.
    DrawRectangle(18, 14, 430, 125, {5, 8, 10, 235});

    const Texture2D portrait = AssetManager::Get().GetTexture("rayden_clean");
    if (portrait.id != 0) {
        DrawTexturePro(
            portrait,
            {0.0f, 0.0f, portrait.width / 4.0f, portrait.height / 4.0f},
            {30.0f, 24.0f, 86.0f, 86.0f},
            {0.0f, 0.0f},
            0.0f,
            WHITE
        );
    }

    DrawText("RAYDEN CRUZ", 128, 23, 23, {190, 220, 255, 255});
    DrawText("LV. 01", 128, 50, 15, LIGHTGRAY);

    DrawRectangle(128, 70, 285, 13, {25, 27, 30, 255});
    DrawRectangle(
        128, 70,
        static_cast<int>(285.0f * player.hp / player.maxHp),
        13,
        {40, 165, 255, 255}
    );
    DrawText("HP", 419, 68, 16, WHITE);

    DrawRectangle(128, 90, 285, 8, {25, 27, 30, 255});
    DrawRectangle(
        128, 90,
        static_cast<int>(285.0f * player.sp / player.maxSp),
        8,
        {65, 205, 255, 255}
    );
    DrawText("SP", 419, 87, 14, WHITE);

    DrawRectangle(128, 105, 285, 8, {25, 27, 30, 255});
    DrawRectangle(
        128, 105,
        static_cast<int>(285.0f * player.rage / player.maxRage),
        8,
        {160, 85, 255, 255}
    );
    DrawText("RAGE", 419, 102, 14, WHITE);
    DrawText("COINS  320", 128, 122, 14, {255, 205, 75, 255});

    DrawRectangle(467, 14, 370, 72, {5, 8, 10, 225});
    DrawText("OLD STEEL YARD // STREET APPROACH", 488, 25, 18, {185, 195, 198, 255});
    DrawText("SLUM DISTRICT // NIGHT SECTOR", 488, 49, 14, {120, 150, 165, 255});
    DrawRectangle(488, 69, 325, 8, {27, 30, 33, 255});

    const float progress = std::clamp(player.position.x / kStageEndX, 0.0f, 1.0f);
    DrawRectangle(
        488, 69,
        static_cast<int>(325.0f * progress),
        8,
        {75, 185, 240, 255}
    );

    DrawRectangle(1046, 14, 216, 82, {5, 8, 10, 230});
    DrawText(
        TextFormat("THREATS %02d", RemainingEnemies()),
        1065, 25, 20, WHITE
    );
    DrawText(
        TextFormat("CLEAR %02d / 14", defeatedEnemies),
        1065, 53, 17, {145, 220, 165, 255}
    );

    if (player.comboCount > 1) {
        DrawText(
            TextFormat("%d HIT", player.comboCount),
            1015, 125, 52,
            {255, 220, 80, 255}
        );
        DrawText("COMBO", 1030, 181, 18, LIGHTGRAY);
    }

    DrawRectangle(18, 651, 620, 51, {5, 8, 10, 220});
    DrawText("WASD MOVE", 35, 664, 15, WHITE);
    DrawText("J PUNCH", 155, 664, 15, WHITE);
    DrawText("K KICK", 245, 664, 15, WHITE);
    DrawText("L ENERGY", 330, 664, 15, {70, 195, 255, 255});
    DrawText("SHIFT DASH", 440, 664, 15, WHITE);
    DrawText("SPACE RAGE", 545, 664, 15, {180, 110, 255, 255});

    DrawText("DISTRICT", 1000, 646, 18, LIGHTGRAY);
    DrawText("FURY", 1073, 646, 29, {225, 60, 70, 255});
    DrawText("STREETS MAKE LEGENDS", 1000, 685, 13, {125, 135, 145, 255});

    if (flowState == GameFlowState::Start) {
        const float pulse = 0.90f + 0.10f * std::sin(static_cast<float>(GetTime()) * 3.0f);
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 155});
        DrawText("DISTRICT FURY", 355, 238, 70, {235, 235, 235, 255});
        DrawText("OLD STEEL YARD", 440, 325, 31, {70, 190, 255, 255});
        DrawText("STREET APPROACH // 01", 468, 362, 17, {180, 190, 195, 255});
        DrawText("A STRONGER CITY. A BRIGHTER YOU.", 420, 407, 20,
                 {static_cast<unsigned char>(210 * pulse), static_cast<unsigned char>(210 * pulse),
                  static_cast<unsigned char>(210 * pulse), 255});
        DrawText("WASD MOVE   J PUNCH   K KICK   L ENERGY   SHIFT DASH   SPACE RAGE",
                 280, 627, 16, LIGHTGRAY);
    } else if (flowState == GameFlowState::Pause) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 175});
        DrawText("PAUSED", 480, 275, 72, WHITE);
        DrawText("ESC  RESUME", 525, 367, 25, LIGHTGRAY);
    } else if (flowState == GameFlowState::GameOver) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 190});
        DrawText("GAME OVER", 430, 260, 65, {240, 70, 75, 255});
        DrawText(TextFormat("MAX COMBO  %d", maxCombo), 510, 354, 26, WHITE);
        DrawText("R  RESTART", 535, 410, 23, LIGHTGRAY);
    } else if (flowState == GameFlowState::Win) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 185});
        DrawText("DISTRICT CLEARED", 372, 250, 59, {255, 215, 75, 255});
        DrawText(TextFormat("MAX COMBO  %d", maxCombo), 510, 340, 26, WHITE);
        DrawText(TextFormat("TIME  %05.1f", elapsedTime), 520, 382, 23, LIGHTGRAY);
        DrawText("R  RESTART", 535, 430, 23, LIGHTGRAY);
    }
}

} // namespace district_fury
