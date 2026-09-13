#include "game/Enemy.h"
#include <cmath>
#include <algorithm>

namespace district_fury {

Enemy::Enemy() {
    Init({800.0f, 500.0f, 0.0f});
}

void Enemy::Init(Vector3D startPos) {
    position = startPos;
    velocity = {0.0f, 0.0f, 0.0f};
    facing = Facing::Left;
    state = EnemyState::Idle;
    maxHp = 50;
    hp = maxHp;
    stateTimer = 0.0f;
}

void Enemy::Update(float dt, const Player& player) {
    if (state == EnemyState::Defeat) {
        stateTimer -= dt;
        return;
    }

    if (state == EnemyState::Hit) {
        stateTimer -= dt;
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        
        // friction
        velocity.x *= 0.9f;
        velocity.y *= 0.9f;
        
        if (stateTimer <= 0.0f) {
            state = EnemyState::Idle;
        }
        return;
    }

    if (state == EnemyState::Attack) {
        stateTimer -= dt;
        if (stateTimer <= 0.0f) {
            state = EnemyState::Idle;
        }
        return;
    }

    // AI Logic (Idle -> Chase -> Attack)
    float dx = player.position.x - position.x;
    float dy = player.position.y - position.y;
    float dist = std::sqrt(dx*dx + dy*dy);
    
    if (state != EnemyState::Attack) {
        facing = (dx > 0) ? Facing::Right : Facing::Left;
    }

    if (dist < 80.0f && std::abs(dy) < 20.0f && player.state != PlayerState::Defeat) {
        // In range to attack
        state = EnemyState::Attack;
        stateTimer = 0.5f; // 500ms attack
        hasHit = false;
    } else if (dist < 400.0f && player.state != PlayerState::Defeat) {
        // Chase
        state = EnemyState::Chase;
        float speed = 150.0f;
        
        if (dist > 0.0f) {
            position.x += (dx / dist) * speed * dt;
            position.y += (dy / dist) * (speed * 0.7f) * dt;
        }
    } else {
        state = EnemyState::Idle;
    }

    // Boundaries
    position.x = std::clamp(position.x, 50.0f, 1230.0f);
    position.y = std::clamp(position.y, 450.0f, 700.0f);
}

void Enemy::TakeDamage(int damage, Vector3D knockback) {
    hp -= damage;
    velocity = knockback;
    if (hp <= 0) {
        hp = 0;
        state = EnemyState::Defeat;
    } else {
        state = EnemyState::Hit;
        stateTimer = 0.5f; // hitstun
    }
}

void Enemy::Draw() const {
    Vector2 screenPos = position.ToScreen();
    
    Color c = (state == EnemyState::Hit) ? WHITE : 
              (state == EnemyState::Attack) ? ORANGE :
              (state == EnemyState::Defeat) ? DARKGRAY : PURPLE;

    // Shadow
    DrawEllipse(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 30.0f, 10.0f, {0, 0, 0, 128});
    
    // Body (Industrial Enemy)
    Rectangle rec = { screenPos.x - 25, screenPos.y - 90, 50, 90 };
    DrawRectangleRec(rec, c);
    
    // Visor/Eye
    float eyeX = (facing == Facing::Right) ? screenPos.x + 15 : screenPos.x - 25;
    DrawRectangle(static_cast<int>(eyeX), static_cast<int>(screenPos.y - 80), 10, 10, RED);
    
    // HP Bar if not defeated
    if (state != EnemyState::Defeat && hp < maxHp) {
        DrawRectangle(static_cast<int>(screenPos.x - 20), static_cast<int>(screenPos.y - 110), 40, 5, RED);
        DrawRectangle(static_cast<int>(screenPos.x - 20), static_cast<int>(screenPos.y - 110), (int)(40.0f * ((float)hp / maxHp)), 5, GREEN);
    }
}

}
