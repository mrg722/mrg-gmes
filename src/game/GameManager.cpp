#include "game/GameManager.h"
#include "raylib.h"
#include "rlgl.h"
#include <cmath>
#include <algorithm>

namespace district_fury {

GameManager::GameManager() : flowState(GameFlowState::Start), hitstopTimer(0), screenshakeTimer(0) {
}

void GameManager::Init() {
    scene.Init();
    player = Player();
    enemy.Init({800.0f, 500.0f, 0.0f});
    flowState = GameFlowState::Combat; 
    damageTexts.clear();
    particles.clear();
    hitstopTimer = 0.0f;
    screenshakeTimer = 0.0f;
}

void GameManager::SpawnDamageText(Vector3D pos, int damage) {
    damageTexts.push_back({pos, damage, 0.5f, 0.5f});
}

void GameManager::SpawnHitParticles(Vector3D pos) {
    for (int i = 0; i < 5; i++) {
        Particle p;
        p.position = pos;
        p.velocity = { (float)GetRandomValue(-200, 200), (float)GetRandomValue(-200, 200) };
        p.timer = 0.3f;
        p.maxTime = 0.3f;
        p.color = {0, 121, 241, 255}; // Blue energy
        particles.push_back(p);
    }
}

void GameManager::DoHitstop(float duration) {
    hitstopTimer = duration;
}

void GameManager::DoScreenshake(float duration) {
    screenshakeTimer = duration;
}

void GameManager::Update(float dt) {
    if (hitstopTimer > 0.0f) {
        hitstopTimer -= dt;
        return; // Hitstop pauses gameplay
    }

    if (screenshakeTimer > 0.0f) {
        screenshakeTimer -= dt;
    }

    // Update VFX
    for (auto& txt : damageTexts) txt.timer -= dt;
    damageTexts.erase(std::remove_if(damageTexts.begin(), damageTexts.end(), [](const DamageText& t) { return t.timer <= 0; }), damageTexts.end());

    for (auto& p : particles) {
        p.timer -= dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.timer <= 0; }), particles.end());

    if (flowState == GameFlowState::Combat) {
        player.Update(dt);
        enemy.Update(dt, player);

        // Combat Collision
        if (player.state == PlayerState::Attack && player.stateTimer > 0.2f && !player.hasHit && enemy.state != EnemyState::Defeat) {
            float dx = enemy.position.x - player.position.x;
            float dy = enemy.position.y - player.position.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            // Check if facing correct dir and in range
            bool facingRight = (player.facing == Facing::Right && dx > 0);
            bool facingLeft = (player.facing == Facing::Left && dx < 0);
            
            if ((facingRight || facingLeft) && dist < 120.0f && std::abs(dy) < 40.0f) {
                // Hit!
                int damage = 10;
                float kbDir = (player.facing == Facing::Right) ? 1.0f : -1.0f;
                enemy.TakeDamage(damage, {kbDir * 300.0f, 0.0f, 0.0f});
                
                player.hasHit = true;
                player.comboCount++;
                SpawnDamageText(enemy.position, damage);
                SpawnHitParticles({enemy.position.x, enemy.position.y, enemy.position.z + 40.0f});
                DoHitstop(0.1f);
                DoScreenshake(0.15f);
            }
        }
        
        // Enemy attack collision
        if (enemy.state == EnemyState::Attack && enemy.stateTimer > 0.3f && !enemy.hasHit && player.state != PlayerState::Defeat) {
            float dx = player.position.x - enemy.position.x;
            float dy = player.position.y - enemy.position.y;
            float dist = std::sqrt(dx*dx + dy*dy);
            
            bool facingRight = (enemy.facing == Facing::Right && dx > 0);
            bool facingLeft = (enemy.facing == Facing::Left && dx < 0);
            
            if ((facingRight || facingLeft) && dist < 120.0f && std::abs(dy) < 40.0f) {
                // Enemy Hits Player
                player.TakeDamage(15);
                enemy.hasHit = true;
                player.comboCount = 0; // Break combo
                float kbDir = (enemy.facing == Facing::Right) ? 1.0f : -1.0f;
                player.velocity = {kbDir * 200.0f, 0.0f, 0.0f};
                
                SpawnDamageText(player.position, 15);
                DoHitstop(0.1f);
                DoScreenshake(0.2f);
            }
        }
        
        if (player.state == PlayerState::Defeat) {
            flowState = GameFlowState::GameOver;
        } else if (enemy.state == EnemyState::Defeat && enemy.stateTimer <= -2.0f) { // wait a bit after defeat
            flowState = GameFlowState::Win;
        }
    } else {
        if (IsKeyPressed(KEY_R)) {
            Init();
        }
    }
}

void GameManager::Draw() const {
    if (screenshakeTimer > 0.0f) {
        float shakeAmt = 5.0f;
        BeginMode2D({ {0,0}, {0,0}, 0.0f, 1.0f });
        // Hack: move camera by doing a global offset. Raylib Camera2D is better, but we do translation.
        rlPushMatrix();
        rlTranslatef(GetRandomValue(-shakeAmt, shakeAmt), GetRandomValue(-shakeAmt, shakeAmt), 0);
    }

    scene.DrawBackground();
    
    // Y-sorting: simple draw order based on depth (Y)
    if (player.position.y < enemy.position.y) {
        player.Draw();
        enemy.Draw();
    } else {
        enemy.Draw();
        player.Draw();
    }
    
    // Draw VFX
    for (const auto& p : particles) {
        Vector2 screenPos = p.position.ToScreen();
        DrawRectangle(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 6, 6, p.color);
    }
    
    for (const auto& txt : damageTexts) {
        Vector2 screenPos = txt.position.ToScreen();
        float yOffset = (1.0f - (txt.timer / txt.maxTime)) * 50.0f; // float up
        DrawText(TextFormat("%d", txt.amount), static_cast<int>(screenPos.x), static_cast<int>(screenPos.y - 100 - yOffset), 30, WHITE);
    }

    scene.DrawForeground();
    
    // HUD
    DrawText("DISTRICT FURY - V0.1", 20, 20, 20, LIGHTGRAY);
    DrawRectangle(20, 50, 300, 25, DARKGRAY);
    DrawRectangle(20, 50, (int)(300.0f * ((float)player.hp / player.maxHp)), 25, BLUE);
    DrawText("RAYDEN", 25, 55, 20, WHITE);
    
    if (enemy.state != EnemyState::Defeat) {
        DrawRectangle(960, 50, 300, 25, DARKGRAY);
        DrawRectangle(960 + 300 - (int)(300.0f * ((float)enemy.hp / enemy.maxHp)), 50, (int)(300.0f * ((float)enemy.hp / enemy.maxHp)), 25, RED);
        DrawText("GRINDER THUG", 1000, 55, 20, WHITE);
    }

    if (player.comboCount > 1) {
        DrawText(TextFormat("%d HITS", player.comboCount), 1100, 200, 40, YELLOW);
    }
    
    if (flowState == GameFlowState::GameOver) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 180});
        DrawText("GAME OVER", 450, 300, 60, RED);
        DrawText("Press R to Restart", 520, 380, 30, WHITE);
    } else if (flowState == GameFlowState::Win) {
        DrawRectangle(0, 0, 1280, 720, {0, 0, 0, 180});
        DrawText("STAGE CLEAR", 450, 300, 60, YELLOW);
        DrawText(TextFormat("Max Combo: %d", player.comboCount), 520, 380, 30, WHITE);
        DrawText("Press R to Restart", 520, 430, 30, WHITE);
    }

    if (screenshakeTimer > 0.0f) {
        rlPopMatrix();
        EndMode2D();
    }
}

}
