#include "game/Player.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

Player::Player() {
    position = {200.0f, 530.0f, 0.0f};
    velocity = {0.0f, 0.0f, 0.0f};
    facing = Facing::Right;
    state = PlayerState::Idle;
    maxHp = 100;
    hp = maxHp;
    maxSp = 100;
    sp = maxSp;
    maxRage = 100;
    rage = 0;
    isRageMode = false;
    stateTimer = 0.0f;
    comboCount = 0;
    hasHit = false;

    Texture2D tex = AssetManager::Get().GetTexture("rayden_sheet");
    animator.Init(tex, 5, 3);
    animator.Play({0, 4, 0.15f, true});
}

void Player::SetState(PlayerState newState) {
    if (state == newState && newState != PlayerState::Attack) return;
    state = newState;
    hasHit = false;

    switch (state) {
        case PlayerState::Idle:
            animator.Play({0, 4, 0.15f, true});
            break;
        case PlayerState::Walk:
            animator.Play({0, 4, 0.10f, true});
            break;
        case PlayerState::Attack:
            // Will be set specifically by Update (punch vs kick)
            stateTimer = 0.30f;
            break;
        case PlayerState::Hit:
            animator.Play({0, 0, 1.0f, false});
            stateTimer = 0.35f;
            break;
        case PlayerState::Defeat:
            animator.Play({0, 0, 1.0f, false});
            break;
    }
}

void Player::Update(float dt) {
    animator.Update(dt);

    // Rage mode timer
    if (isRageMode) {
        stateTimer -= dt;  // reuse a separate counter would be better, but keep simple
        // We track rage duration via a simple approach: rage drains over time
        rage -= static_cast<int>(20.0f * dt); // drains in ~5 seconds
        if (rage <= 0) {
            rage = 0;
            isRageMode = false;
        }
    }

    if (state == PlayerState::Defeat) return;

    if (state == PlayerState::Hit) {
        stateTimer -= dt;
        position.x += velocity.x * dt;
        velocity.x *= 0.92f;
        if (stateTimer <= 0.0f) {
            SetState(PlayerState::Idle);
        }
        position.x = std::clamp(position.x, 50.0f, 1230.0f);
        return;
    }

    if (state == PlayerState::Attack) {
        stateTimer -= dt;
        if (stateTimer <= 0.0f || animator.isFinished) {
            SetState(PlayerState::Idle);
        }
        return;
    }

    // Movement
    float baseSpeed = 250.0f;
    bool dashing = IsKeyDown(KEY_LEFT_SHIFT);
    float speed = dashing ? 420.0f : baseSpeed;

    Vector2 moveDir = {0.0f, 0.0f};
    if (IsKeyDown(KEY_W)) moveDir.y = -1.0f;
    if (IsKeyDown(KEY_S)) moveDir.y = 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x = -1.0f;
    if (IsKeyDown(KEY_D)) moveDir.x = 1.0f;

    if (moveDir.x != 0.0f || moveDir.y != 0.0f) {
        if (state != PlayerState::Walk) SetState(PlayerState::Walk);
        float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        position.x += (moveDir.x / len) * speed * dt;
        position.y += (moveDir.y / len) * (speed * 0.7f) * dt;

        if (moveDir.x < 0.0f) facing = Facing::Left;
        if (moveDir.x > 0.0f) facing = Facing::Right;
    } else {
        if (state == PlayerState::Walk) SetState(PlayerState::Idle);
    }

    position.x = std::clamp(position.x, 50.0f, 1230.0f);
    position.y = std::clamp(position.y, 450.0f, 700.0f);

    // Attacks — J = punch, K = kick, L = energy
    if (IsKeyPressed(KEY_J)) {
        state = PlayerState::Attack;
        hasHit = false;
        stateTimer = 0.30f;
        animator.Play({5, 9, 0.06f, false}); // punch row
    } else if (IsKeyPressed(KEY_K)) {
        state = PlayerState::Attack;
        hasHit = false;
        stateTimer = 0.30f;
        animator.Play({10, 14, 0.06f, false}); // kick row
    } else if (IsKeyPressed(KEY_L) && sp >= 20) {
        sp -= 20;
        state = PlayerState::Attack;
        hasHit = false;
        stateTimer = 0.35f;
        animator.Play({5, 9, 0.07f, false}); // energy uses punch frames
    }

    // Rage activation
    if (IsKeyPressed(KEY_SPACE) && rage >= maxRage && !isRageMode) {
        isRageMode = true;
        // rage will drain in Update
    }

    // SP regen
    if (sp < maxSp) {
        sp = std::min(maxSp, sp + static_cast<int>(5.0f * dt));
    }
}

void Player::TakeDamage(int damage) {
    if (state == PlayerState::Hit || state == PlayerState::Defeat) return;
    hp -= damage;
    rage = std::min(maxRage, rage + 10);
    if (hp <= 0) {
        hp = 0;
        SetState(PlayerState::Defeat);
    } else {
        SetState(PlayerState::Hit);
    }
}

void Player::Draw() const {
    Vector2 screenPos = position.ToScreen();

    // Shadow
    DrawEllipse(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y),
                35.0f, 12.0f, {0, 0, 0, 150});

    if (animator.texture.id != 0) {
        float scale = 1.5f;
        Color tint = WHITE;
        if (state == PlayerState::Hit) tint = RED;
        else if (isRageMode) tint = {150, 200, 255, 255};
        animator.Draw(screenPos, scale, facing == Facing::Left, tint);
    } else {
        // Fallback rectangle
        Color c = (state == PlayerState::Hit)    ? RED
                : (state == PlayerState::Attack)  ? YELLOW
                : (state == PlayerState::Defeat)  ? DARKGRAY
                                                  : BLUE;
        Rectangle rec = {screenPos.x - 20, screenPos.y - 80, 40, 80};
        DrawRectangleRec(rec, c);
        float eyeX = (facing == Facing::Right) ? screenPos.x + 10 : screenPos.x - 20;
        DrawRectangle(static_cast<int>(eyeX), static_cast<int>(screenPos.y - 70), 10, 10, SKYBLUE);
    }
}

} // namespace district_fury
