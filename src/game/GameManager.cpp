#include "game/GameManager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

GameManager::GameManager()
    : flowState(GameFlowState::Start),
      hitstopTimer(0.0f),
      screenshakeTimer(0.0f),
      introTimer(1.5f),
      comboTimer(0.0f),
      elapsedTime(0.0f),
      maxCombo(0) {
}

void GameManager::Init() {
    scene.Init();
    player.Reset();
    enemy.Init({820.0f, 535.0f, 0.0f});
    flowState = GameFlowState::Start;
    introTimer = 1.5f;
    hitstopTimer = 0.0f;
    screenshakeTimer = 0.0f;
    comboTimer = 0.0f;
    elapsedTime = 0.0f;
    maxCombo = 0;
    damageTexts.clear();
    particles.clear();
}

void GameManager::SpawnDamageText(Vector3D pos, int damage, bool critical) {
    damageTexts.push_back({pos, damage, 0.65f, 0.65f, critical});
}

void GameManager::SpawnHitParticles(Vector3D pos, AttackType type, bool heavy) {
    const int count = heavy ? 18 : 10;
    Color color = type == AttackType::Energy ? Color{45, 180, 255, 255} :
                  type == AttackType::Kick ? Color{255, 170, 55, 255} :
                  Color{255, 235, 150, 255};
    for (int i = 0; i < count; ++i) {
        const float angle = (static_cast<float>(i) / static_cast<float>(count)) * 6.2831853f;
        const float speed = static_cast<float>(GetRandomValue(90, heavy ? 320 : 230));
        Particle p;
        p.position = {pos.x, pos.y, pos.z};
        p.velocity = {std::cos(angle) * speed, std::sin(angle) * speed};
        p.timer = heavy ? 0.48f : 0.30f;
        p.maxTime = p.timer;
        p.color = color;
        p.size = static_cast<float>(GetRandomValue(3, heavy ? 8 : 6));
        particles.push_back(p);
    }

    for (int i = 0; i < (heavy ? 3 : 2); ++i) {
        Particle arc;
        arc.position = {pos.x, pos.y, pos.z};
        arc.velocity = {static_cast<float>(GetRandomValue(-180, 180)), static_cast<float>(GetRandomValue(-90, 90))};
        arc.timer = 0.20f + i * 0.04f;
        arc.maxTime = arc.timer;
        arc.color = type == AttackType::Energy ? Color{0, 210, 255, 255} : Color{255, 120, 45, 255};
        arc.size = static_cast<float>(GetRandomValue(8, 14));
        particles.push_back(arc);
    }
}

void GameManager::DoHitstop(float duration) {
    hitstopTimer = std::max(hitstopTimer, duration);
}

void GameManager::DoScreenshake(float duration) {
    screenshakeTimer = std::max(screenshakeTimer, duration);
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
    enemy.Update(dt, player);

    if (player.AttackIsActive() && !player.hasHit && enemy.state != EnemyState::Defeat) {
        const float dx = enemy.position.x - player.position.x;
        const float dy = enemy.position.y - player.position.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        const bool facingCorrect = (player.facing == Facing::Right && dx > 0.0f) || (player.facing == Facing::Left && dx < 0.0f);
        const float range = player.GetAttackRange();

        if (facingCorrect && std::abs(dy) <= player.GetAttackDepthRange() && dist <= range) {
            const int damage = player.GetAttackDamage();
            const bool critical = player.attackType == AttackType::Kick || (player.comboStep >= 2 && player.attackType == AttackType::Punch);
            const float dir = player.facing == Facing::Right ? 1.0f : -1.0f;
            enemy.TakeDamage(damage, {dir * player.GetAttackKnockback(), (dy > 0.0f ? 25.0f : -25.0f), 0.0f});
            player.hasHit = true;
            player.comboCount++;
            player.comboWindow = 0.9f;
            maxCombo = std::max(maxCombo, player.comboCount);
            SpawnDamageText(enemy.position, damage, critical);
            SpawnHitParticles({enemy.position.x, enemy.position.y, enemy.position.z + 55.0f}, player.attackType, critical);
            DoHitstop(player.attackType == AttackType::Energy ? 0.12f : 0.08f);
            DoScreenshake(critical ? 0.16f : 0.10f);
        }
    }

    if (enemy.AttackIsActive() && !enemy.hasHit && player.state != PlayerState::Defeat) {
        const float dx = player.position.x - enemy.position.x;
        const float dy = player.position.y - enemy.position.y;
        const float dist = std::sqrt(dx * dx + dy * dy);
        const bool facingCorrect = (enemy.facing == Facing::Right && dx > 0.0f) || (enemy.facing == Facing::Left && dx < 0.0f);
        if (facingCorrect && std::abs(dy) <= 38.0f && dist <= 105.0f) {
            player.TakeDamage(14);
            enemy.hasHit = true;
            player.velocity = {(enemy.facing == Facing::Right ? 1.0f : -1.0f) * 240.0f, 0.0f, 0.0f};
            player.comboCount = 0;
            player.comboWindow = 0.0f;
            SpawnDamageText(player.position, 14, false);
            SpawnHitParticles({player.position.x, player.position.y, player.position.z + 45.0f}, AttackType::Punch, false);
            DoHitstop(0.08f);
            DoScreenshake(0.14f);
        }
    }

    if (enemy.state == EnemyState::Defeat && enemy.stateTimer <= 0.0f) {
        flowState = GameFlowState::Win;
    } else if (player.state == PlayerState::Defeat) {
        flowState = GameFlowState::GameOver;
    }
}

void GameManager::Draw() const {
    Camera2D camera{};
    camera.offset = {640.0f, 360.0f};
    camera.target = {640.0f, 360.0f};
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    if (screenshakeTimer > 0.0f) {
        const float strength = 5.0f;
        camera.target.x -= static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * strength;
        camera.target.y -= static_cast<float>(GetRandomValue(-100, 100)) / 100.0f * strength;
    }

    BeginMode2D(camera);
    scene.DrawBackground();

    if (player.position.y < enemy.position.y) {
        player.Draw();
        enemy.Draw();
    } else {
        enemy.Draw();
        player.Draw();
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
                 static_cast<int>(pos.x - 25), static_cast<int>(pos.y - 108.0f - progress * 55.0f), size, color);
    }

    scene.DrawForeground();
    EndMode2D();

    // HUD
    DrawRectangle(18, 16, 430, 112, {8, 11, 12, 210});
    DrawText("RAYDEN", 32, 24, 25, {190, 220, 255, 255});
    DrawRectangle(32, 56, 360, 16, {30, 30, 32, 255});
    DrawRectangle(32, 56, static_cast<int>(360.0f * player.hp / player.maxHp), 16, {35, 155, 255, 255});
    DrawText("HP", 400, 53, 18, WHITE);
    DrawRectangle(32, 81, 360, 10, {30, 30, 32, 255});
    DrawRectangle(32, 81, static_cast<int>(360.0f * player.sp / player.maxSp), 10, {65, 205, 255, 255});
    DrawText("SP", 400, 77, 16, WHITE);
    DrawRectangle(32, 99, 360, 10, {30, 30, 32, 255});
    DrawRectangle(32, 99, static_cast<int>(360.0f * player.rage / player.maxRage), 10, {150, 90, 255, 255});
    DrawText("RAGE", 400, 95, 16, WHITE);

    if (enemy.state != EnemyState::Defeat) {
        DrawRectangle(825, 16, 437, 70, {8, 11, 12, 210});
        DrawText("GRINDER THUG", 910, 24, 23, WHITE);
        DrawRectangle(860, 56, 360, 15, {30, 30, 32, 255});
        DrawRectangle(860, 56, static_cast<int>(360.0f * enemy.hp / enemy.maxHp), 15, {225, 55, 70, 255});
    }

    DrawText("OLD STEEL YARD", 520, 26, 20, {170, 180, 175, 255});

    if (player.comboCount > 1) {
        DrawText(TextFormat("%d HIT", player.comboCount), 1030, 175, 54, {255, 220, 80, 255});
        DrawText("COMBO", 1040, 230, 20, LIGHTGRAY);
    }

    if (flowState == GameFlowState::Start) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 115});
        DrawText("OLD STEEL YARD", 405, 270, 58, WHITE);
        DrawText("DISTRICT FURY", 490, 335, 30, {120, 190, 255, 255});
        DrawText("WASD  MOVE    J  PUNCH    K  KICK    L  ENERGY    SHIFT  DASH    SPACE  RAGE",
                 175, 630, 18, LIGHTGRAY);
    } else if (flowState == GameFlowState::Pause) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 180});
        DrawText("PAUSED", 495, 285, 70, WHITE);
        DrawText("ESC  RESUME", 535, 375, 25, LIGHTGRAY);
    } else if (flowState == GameFlowState::GameOver) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 185});
        DrawText("GAME OVER", 425, 285, 65, {240, 70, 75, 255});
        DrawText(TextFormat("MAX COMBO  %d", maxCombo), 515, 375, 26, WHITE);
        DrawText("R  RESTART", 535, 425, 23, LIGHTGRAY);
    } else if (flowState == GameFlowState::Win) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 185});
        DrawText("STAGE CLEAR", 420, 250, 64, {255, 215, 75, 255});
        DrawText(TextFormat("MAX COMBO  %d", maxCombo), 510, 340, 28, WHITE);
        DrawText(TextFormat("TIME  %05.1f", elapsedTime), 525, 385, 24, LIGHTGRAY);
        DrawText("R  RESTART", 535, 445, 23, LIGHTGRAY);
    }
}

}
