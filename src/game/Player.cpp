#include "game/Player.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

Player::Player() {
    position = { 200.0f, 500.0f, 0.0f };
    velocity = { 0.0f, 0.0f, 0.0f };
    facing = Facing::Right;
    state = PlayerState::Idle;
    maxHp = 100;
    hp = maxHp;
    stateTimer = 0.0f;
    comboCount = 0;
}

void Player::Update(float dt) {
    if (state == PlayerState::Defeat) return;

    if (state == PlayerState::Hit) {
        stateTimer -= dt;
        if (stateTimer <= 0.0f) {
            state = PlayerState::Idle;
        }
        // apply knockback friction?
        position.x += velocity.x * dt;
        return;
    }

    if (state == PlayerState::Attack) {
        stateTimer -= dt;
        if (stateTimer <= 0.0f) {
            state = PlayerState::Idle;
        }
        return;
    }

    // Input Movement
    float speed = 300.0f;
    Vector2 moveDir = { 0.0f, 0.0f };
    
    if (IsKeyDown(KEY_W)) moveDir.y = -1.0f;
    if (IsKeyDown(KEY_S)) moveDir.y = 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x = -1.0f;
    if (IsKeyDown(KEY_D)) moveDir.x = 1.0f;
    
    if (moveDir.x != 0.0f || moveDir.y != 0.0f) {
        state = PlayerState::Walk;
        // Normalize
        float len = std::sqrt(moveDir.x*moveDir.x + moveDir.y*moveDir.y);
        moveDir.x /= len;
        moveDir.y /= len;
        
        position.x += moveDir.x * speed * dt;
        position.y += moveDir.y * (speed * 0.7f) * dt; // slower in depth
        
        if (moveDir.x < 0.0f) facing = Facing::Left;
        if (moveDir.x > 0.0f) facing = Facing::Right;
    } else {
        state = PlayerState::Idle;
    }

    // Boundaries
    position.x = std::clamp(position.x, 50.0f, 1230.0f);
    position.y = std::clamp(position.y, 450.0f, 700.0f);

    // Attacks placeholder
    if (IsKeyPressed(KEY_J) || IsKeyPressed(KEY_K)) {
        state = PlayerState::Attack;
        stateTimer = 0.3f; // 300ms attack
        hasHit = false;
    }
}

void Player::TakeDamage(int damage) {
    hp -= damage;
    if (hp <= 0) {
        hp = 0;
        state = PlayerState::Defeat;
    } else {
        state = PlayerState::Hit;
        stateTimer = 0.4f; // hitstun
    }
}

void Player::Draw() const {
    Vector2 screenPos = position.ToScreen();
    
    Color c = (state == PlayerState::Hit) ? RED : 
              (state == PlayerState::Attack) ? YELLOW :
              (state == PlayerState::Defeat) ? DARKGRAY : BLUE;

    // Draw shadow
    DrawEllipse(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 30.0f, 10.0f, {0, 0, 0, 128});
    
    // Draw body
    Rectangle rec = { screenPos.x - 20, screenPos.y - 80, 40, 80 };
    DrawRectangleRec(rec, c);
    
    // Draw facing indicator (visor)
    float eyeX = (facing == Facing::Right) ? screenPos.x + 10 : screenPos.x - 20;
    DrawRectangle(static_cast<int>(eyeX), static_cast<int>(screenPos.y - 70), 10, 10, SKYBLUE);
}
}
