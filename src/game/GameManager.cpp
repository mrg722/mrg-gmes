#include "game/GameManager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace district_fury {
namespace {

struct SpawnDefinition {
    float x;
    float y;
    StreetEnemyType type;
};

constexpr SpawnDefinition kStageSpawns[] = {
    {900.0f, 565.0f, StreetEnemyType::Punk},
    {1130.0f, 520.0f, StreetEnemyType::Punk},
    {1470.0f, 600.0f, StreetEnemyType::Charger},
    {1730.0f, 545.0f, StreetEnemyType::Brute},
    {2250.0f, 585.0f, StreetEnemyType::Punk},
    {2480.0f, 520.0f, StreetEnemyType::Charger},
    {2750.0f, 610.0f, StreetEnemyType::Brute},
    {3320.0f, 550.0f, StreetEnemyType::Punk},
    {3570.0f, 625.0f, StreetEnemyType::Charger},
    {3880.0f, 520.0f, StreetEnemyType::Brute},
    {4380.0f, 585.0f, StreetEnemyType::Punk},
    {4650.0f, 525.0f, StreetEnemyType::Punk},
    {5050.0f, 610.0f, StreetEnemyType::Brute},
    {5480.0f, 555.0f, StreetEnemyType::Enforcer}
};

float Length(const Vector2& v) {
    return std::sqrt(v.x * v.x + v.y * v.y);
}

}

GameManager::GameManager()
    : flowState(GameFlowState::Start),
      hitstopTimer(0.0f),
      screenshakeTimer(0.0f),
      introTimer(1.8f),
      elapsedTime(0.0f),
      cameraX(kViewportWidth * 0.5f),
      maxCombo(0),
      defeatedEnemies(0) {
}

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
    introTimer = 1.8f;
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
    const int count = heavy ? 20 : 11;
    const Color color = type == AttackType::Energy ? Color{45, 180, 255, 255} :
                        type == AttackType::Kick ? Color{255, 170, 55, 255} :
                        Color{255, 235, 150, 255};
    for (int i = 0; i < count; ++i) {
        const float angle = (static_cast<float>(i) / static_cast<float>(count)) * 6.2831853f;
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
        for (int i = 0; i < 4; ++i) {
            Particle arc;
            arc.position = pos;
            arc.velocity = {static_cast<float>(GetRandomValue(-210, 210)), static_cast<float>(GetRandomValue(-120, 120))};
            arc.timer = 0.22f + i * 0.03f;
            arc.maxTime = arc.timer;
            arc.color = type == AttackType::Energy ? Color{0, 210, 255, 255} : Color{255, 120, 45, 255};
            arc.size = static_cast<float>(GetRandomValue(8, 14));
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
    return static_cast<int>(std::count_if(enemies.begin(), enemies.end(), [](const StreetEnemy& enemy) {
        return !enemy.IsDefeated();
    }));
}

bool GameManager::AllEnemiesDefeated() const {
    return RemainingEnemies() == 0;
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

    if (screenshakeTimer > 0.0f) screenshakeTimer = std::max(0.0f, screenshakeTimer - dt);
    elapsedTime += dt;

    for (auto& txt : damageTexts) txt.timer -= dt;
    damageTexts.erase(std::remove_if(damageTexts.begin(), damageTexts.end(), [](const DamageText& t) { return t.timer <= 0.0f; }), damageTexts.end());

    for (auto& p : particles) {
        p.timer -= dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.velocity.x *= 0.93f;
        p.velocity.y *= 0.93f;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.timer <= 0.0f; }), particles.end());

    player.Update(dt);

    // Activate enemies progressively as the player reaches their street encounter.
    for (auto& enemy : enemies) {
        if (!enemy.active && player.position.x + 720.0f >= enemy.position.x) enemy.Activate();
    }

    for (auto& enemy : enemies) enemy.Update(dt, player);

    // Player attacks use explicit world hitboxes rather than raw center-to-center distance.
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
            const bool critical = player.attackType == AttackType::Kick ||
                                  (player.comboStep >= 2 && player.attackType == AttackType::Punch);
            const float direction = player.facing == Facing::Right ? 1.0f : -1.0f;
            enemy.TakeDamage(damage, {direction * player.GetAttackKnockback(), 0.0f, 0.0f});
            player.hasHit = true;
            player.comboCount++;
            player.comboWindow = 0.9f;
            maxCombo = std::max(maxCombo, player.comboCount);
            SpawnDamageText(enemy.position, damage, critical);
            SpawnHitParticles({enemy.position.x, enemy.position.y - 58.0f, enemy.position.z}, player.attackType, critical);
            DoHitstop(player.attackType == AttackType::Energy ? 0.12f : 0.075f);
            DoScreenshake(critical ? 0.15f : 0.09f);
        }
    }

    // Only one active enemy can score a melee hit in a frame; this keeps crowds fair and readable.
    if (player.state != PlayerState::Defeat) {
        for (auto& enemy : enemies) {
            if (!enemy.active || enemy.IsDefeated() || enemy.hasHit) continue;
            if (!enemy.AttackIsActive()) continue;
            if (!enemy.GetAttackHitbox().Intersects(player.GetHurtbox())) continue;

            const int damage = static_cast<int>(enemy.attackDamage);
            player.TakeDamage(damage);
            enemy.hasHit = true;
            player.velocity = {(enemy.facing == Facing::Right ? 1.0f : -1.0f) * 240.0f, 0.0f, 0.0f};
            player.comboCount = 0;
            player.comboWindow = 0.0f;
            SpawnDamageText(player.position, damage, false);
            SpawnHitParticles({player.position.x, player.position.y - 48.0f, 0.0f}, AttackType::Punch, false);
            DoHitstop(0.08f);
            DoScreenshake(0.12f);
            break;
        }
    }

    // Count deaths exactly once using the current hp/state transition.
    defeatedEnemies = static_cast<int>(std::count_if(enemies.begin(), enemies.end(), [](const StreetEnemy& enemy) {
        return enemy.IsDefeated();
    }));

    if (player.position.x >= kStageEndX - 220.0f && AllEnemiesDefeated()) {
        flowState = GameFlowState::Win;
    } else if (player.state == PlayerState::Defeat) {
        flowState = GameFlowState::GameOver;
    }

    // Camera follows the player with a generous dead-zone and hard stage boundaries.
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
        if (enemies[i].active) drawOrder.push_back({enemies[i].position.y, static_cast<int>(i), false});
    }
    std::sort(drawOrder.begin(), drawOrder.end(), [](const DrawItem& a, const DrawItem& b) { return a.depth < b.depth; });

    for (const auto& item : drawOrder) {
        if (item.playerItem) player.Draw();
        else enemies[static_cast<std::size_t>(item.index)].Draw();
    }

    for (const auto& p : particles) {
        const Vector2 pos = p.position.ToScreen();
        const float alpha = std::clamp(p.timer / p.maxTime, 0.0f, 1.0f);
        Color c = p.color;
        c.a = static_cast<unsigned char>(255.0f * alpha);
        DrawCircle(static_cast<int>(pos.x), static_cast<int>(pos.y), p.size * alpha + 1.0f, c);
    }

    for (const auto& txt : damageTexts) {
        const Vector2 pos = txt.position.ToScreen();
        const float progress = 1.0f - (txt.timer / txt.maxTime);
        const int size = txt.critical ? 34 : 28;
        const Color color = txt.critical ? Color{255, 195, 70, 255} : WHITE;
        DrawText(TextFormat("%s%d", txt.critical ? "CRIT " : "", txt.amount),
                 static_cast<int>(pos.x - 25), static_cast<int>(pos.y - 105.0f - progress * 55.0f), size, color);
    }

    scene.DrawForeground(cameraX);
    EndMode2D();

    // Persistent HUD.
    DrawRectangle(18, 16, 405, 116, {8, 11, 12, 220});
    DrawText("RAYDEN CRUZ", 32, 24, 23, {190, 220, 255, 255});
    DrawRectangle(32, 56, 330, 15, {30, 30, 32, 255});
    DrawRectangle(32, 56, static_cast<int>(330.0f * player.hp / player.maxHp), 15, {35, 155, 255, 255});
    DrawText("HP", 372, 52, 18, WHITE);
    DrawRectangle(32, 79, 330, 9, {30, 30, 32, 255});
    DrawRectangle(32, 79, static_cast<int>(330.0f * player.sp / player.maxSp), 9, {65, 205, 255, 255});
    DrawText("SP", 372, 75, 16, WHITE);
    DrawRectangle(32, 96, 330, 9, {30, 30, 32, 255});
    DrawRectangle(32, 96, static_cast<int>(330.0f * player.rage / player.maxRage), 9, {150, 90, 255, 255});
    DrawText("RAGE", 372, 92, 16, WHITE);

    DrawRectangle(465, 18, 350, 58, {8, 11, 12, 180});
    DrawText("OLD STEEL YARD // STREET APPROACH", 484, 30, 18, {170, 185, 188, 255});
    DrawRectangle(485, 55, 310, 8, {30, 30, 32, 255});
    const float progress = std::clamp(player.position.x / kStageEndX, 0.0f, 1.0f);
    DrawRectangle(485, 55, static_cast<int>(310.0f * progress), 8, {90, 180, 220, 255});

    DrawRectangle(1040, 16, 222, 70, {8, 11, 12, 205});
    DrawText(TextFormat("THREATS  %02d", RemainingEnemies()), 1060, 27, 20, {235, 235, 235, 255});
    DrawText(TextFormat("CLEAR  %02d", defeatedEnemies), 1060, 55, 17, {155, 205, 165, 255});

    if (player.comboCount > 1) {
        DrawText(TextFormat("%d HIT", player.comboCount), 1030, 150, 54, {255, 220, 80, 255});
        DrawText("COMBO", 1040, 208, 19, LIGHTGRAY);
    }

    if (flowState == GameFlowState::Start) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 125});
        DrawText("OLD STEEL YARD", 395, 245, 56, WHITE);
        DrawText("STREET APPROACH", 463, 313, 30, {120, 190, 255, 255});
        DrawText("MOVE THROUGH THE DISTRICT", 450, 370, 22, LIGHTGRAY);
        DrawText("WASD  MOVE   J  PUNCH   K  KICK   L  ENERGY   SHIFT  DASH   SPACE  RAGE",
                 166, 630, 18, LIGHTGRAY);
    } else if (flowState == GameFlowState::Pause) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 180});
        DrawText("PAUSED", 495, 285, 70, WHITE);
        DrawText("ESC  RESUME", 535, 375, 25, LIGHTGRAY);
    } else if (flowState == GameFlowState::GameOver) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 185});
        DrawText("GAME OVER", 425, 270, 65, {240, 70, 75, 255});
        DrawText(TextFormat("MAX COMBO  %d", maxCombo), 515, 365, 26, WHITE);
        DrawText("R  RESTART", 535, 425, 23, LIGHTGRAY);
    } else if (flowState == GameFlowState::Win) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 185});
        DrawText("DISTRICT CLEARED", 375, 245, 60, {255, 215, 75, 255});
        DrawText(TextFormat("MAX COMBO  %d", maxCombo), 510, 335, 28, WHITE);
        DrawText(TextFormat("TIME  %05.1f", elapsedTime), 535, 380, 24, LIGHTGRAY);
        DrawText("R  RESTART", 535, 440, 23, LIGHTGRAY);
    }
}

}
