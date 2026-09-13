#include "game/Player.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>

namespace district_fury {

namespace {
constexpr float kMinX = 50.0f;
constexpr float kMaxX = 1230.0f;
constexpr float kMinLane = 450.0f;
constexpr float kMaxLane = 700.0f;
}

Player::Player() {
    Reset();
}

void Player::Reset() {
    position = {200.0f, 530.0f, 0.0f};
    velocity = {0.0f, 0.0f, 0.0f};
    facing = Facing::Right;
    state = PlayerState::Idle;
    attackType = AttackType::None;

    maxHp = 100;
    hp = maxHp;
    maxSp = 100;
    sp = maxSp;
    maxRage = 100;
    rage = 0;
    isRageMode = false;

    stateTimer = 0.0f;
    attackElapsed = 0.0f;
    attackDuration = 0.0f;
    dashTimer = 0.0f;
    comboWindow = 0.0f;
    spRegenAccumulator = 0.0f;
    rageDrainAccumulator = 0.0f;
    comboCount = 0;
    comboStep = 0;
    hasHit = false;

    animator = Animator{};
}

void Player::SetState(PlayerState newState) {
    if (state == newState && newState != PlayerState::Attack) return;
    state = newState;
    hasHit = false;

    switch (state) {
        case PlayerState::Idle:
            animator.Play({0, 4, 0.12f, true});
            break;
        case PlayerState::Walk:
            animator.Play({0, 4, 0.09f, true});
            break;
        case PlayerState::Dash:
            animator.Play({0, 4, 0.05f, true});
            break;
        case PlayerState::Hit:
            animator.Play({0, 4, 0.08f, false});
            stateTimer = 0.28f;
            break;
        case PlayerState::Defeat:
            animator.Play({0, 4, 0.10f, false});
            break;
        case PlayerState::Attack:
            break;
    }
}

static void EnsurePlayerAnimator(Animator& animator) {
    if (animator.texture.id == 0) {
        Texture2D texture = AssetManager::Get().GetTexture("rayden_sheet");
        if (texture.id != 0) {
            animator.Init(texture, 5, 3);
            animator.Play({0, 4, 0.12f, true});
        }
    }
}

void Player::Update(float dt) {
    EnsurePlayerAnimator(animator);
    animator.Update(dt);

    if (comboWindow > 0.0f) {
        comboWindow -= dt;
        if (comboWindow <= 0.0f) {
            comboWindow = 0.0f;
            comboCount = 0;
            comboStep = 0;
        }
    }

    if (isRageMode) {
        rageDrainAccumulator += dt;
        if (rageDrainAccumulator >= 0.05f) {
            const int drain = std::max(1, static_cast<int>(rageDrainAccumulator * 20.0f));
            rage = std::max(0, rage - drain);
            rageDrainAccumulator = 0.0f;
        }
        if (rage <= 0) {
            rage = 0;
            isRageMode = false;
        }
    }

    if (state == PlayerState::Defeat) return;

    if (state == PlayerState::Hit) {
        stateTimer -= dt;
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        velocity.x *= 0.88f;
        velocity.y *= 0.88f;
        if (stateTimer <= 0.0f) SetState(PlayerState::Idle);
        position.x = std::clamp(position.x, kMinX, kMaxX);
        position.y = std::clamp(position.y, kMinLane, kMaxLane);
        return;
    }

    if (state == PlayerState::Dash) {
        dashTimer -= dt;
        const float dashSpeed = 700.0f;
        const float direction = facing == Facing::Right ? 1.0f : -1.0f;
        position.x += direction * dashSpeed * dt;
        position.x = std::clamp(position.x, kMinX, kMaxX);
        if (dashTimer <= 0.0f) SetState(PlayerState::Idle);
        return;
    }

    if (state == PlayerState::Attack) {
        attackElapsed += dt;
        stateTimer -= dt;
        if (stateTimer <= 0.0f || animator.isFinished) {
            SetState(PlayerState::Idle);
        }
        return;
    }

    Vector2 moveDir = {0.0f, 0.0f};
    if (IsKeyDown(KEY_W)) moveDir.y -= 1.0f;
    if (IsKeyDown(KEY_S)) moveDir.y += 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveDir.x += 1.0f;

    const bool moving = moveDir.x != 0.0f || moveDir.y != 0.0f;
    const float baseSpeed = 250.0f;

    if (moving) {
        const float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        const float speed = baseSpeed;
        position.x += (moveDir.x / len) * speed * dt;
        position.y += (moveDir.y / len) * speed * 0.70f * dt;
        if (moveDir.x < 0.0f) facing = Facing::Left;
        if (moveDir.x > 0.0f) facing = Facing::Right;
        if (state != PlayerState::Walk) SetState(PlayerState::Walk);
    } else if (state == PlayerState::Walk) {
        SetState(PlayerState::Idle);
    }

    if (IsKeyPressed(KEY_LEFT_SHIFT)) {
        dashTimer = 0.12f;
        SetState(PlayerState::Dash);
        return;
    }

    auto beginAttack = [&](AttackType type, float duration, int startFrame, int endFrame, float frameDuration) {
        state = PlayerState::Attack;
        attackType = type;
        attackElapsed = 0.0f;
        attackDuration = duration;
        stateTimer = duration;
        hasHit = false;
        animator.Play({startFrame, endFrame, frameDuration, false});
    };

    if (IsKeyPressed(KEY_J)) {
        comboStep = (comboWindow > 0.0f) ? (comboStep + 1) % 3 : 0;
        beginAttack(AttackType::Punch, 0.30f, 5, 9, 0.06f);
        return;
    }

    if (IsKeyPressed(KEY_K)) {
        comboStep = 3;
        beginAttack(AttackType::Kick, 0.36f, 10, 14, 0.055f);
        return;
    }

    if (IsKeyPressed(KEY_L) && sp >= 20) {
        sp -= 20;
        beginAttack(AttackType::Energy, 0.46f, 5, 9, 0.07f);
        return;
    }

    if (IsKeyPressed(KEY_SPACE) && rage >= maxRage && !isRageMode) {
        isRageMode = true;
        rageDrainAccumulator = 0.0f;
        return;
    }

    spRegenAccumulator += dt;
    while (spRegenAccumulator >= 0.2f && sp < maxSp) {
        ++sp;
        spRegenAccumulator -= 0.2f;
    }
}

bool Player::AttackIsActive() const {
    if (state != PlayerState::Attack) return false;
    if (attackType == AttackType::Punch) return attackElapsed >= 0.085f && attackElapsed <= 0.205f;
    if (attackType == AttackType::Kick) return attackElapsed >= 0.10f && attackElapsed <= 0.255f;
    if (attackType == AttackType::Energy) return attackElapsed >= 0.16f && attackElapsed <= 0.36f;
    return false;
}

int Player::GetAttackDamage() const {
    int damage = 10;
    if (attackType == AttackType::Kick) damage = 14;
    if (attackType == AttackType::Energy) damage = 18;
    if (comboStep >= 2 && attackType == AttackType::Punch) damage += 4;
    if (isRageMode) damage = static_cast<int>(std::round(damage * 1.35f));
    return damage;
}

float Player::GetAttackRange() const {
    if (attackType == AttackType::Kick) return 135.0f;
    if (attackType == AttackType::Energy) return 220.0f;
    return 115.0f;
}

float Player::GetAttackDepthRange() const {
    return attackType == AttackType::Energy ? 55.0f : 42.0f;
}

float Player::GetAttackKnockback() const {
    if (attackType == AttackType::Kick) return 420.0f;
    if (attackType == AttackType::Energy) return 500.0f;
    return 300.0f;
}

void Player::TakeDamage(int damage) {
    if (state == PlayerState::Hit || state == PlayerState::Defeat) return;
    hp = std::max(0, hp - damage);
    rage = std::min(maxRage, rage + 15);
    if (hp == 0) {
        SetState(PlayerState::Defeat);
    } else {
        SetState(PlayerState::Hit);
    }
}

void Player::Draw() const {
    const Vector2 screenPos = position.ToScreen();
    DrawEllipse(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y),
                42.0f, 13.0f, {0, 0, 0, 150});

    if (animator.texture.id != 0) {
        float scale = 1.45f;
        Color tint = WHITE;
        if (state == PlayerState::Hit) tint = {255, 190, 190, 255};
        if (isRageMode) tint = {190, 220, 255, 255};
        animator.Draw(screenPos, scale, facing == Facing::Left, tint);

        if (isRageMode) {
            const float pulse = 48.0f + std::sin(static_cast<float>(GetTime()) * 10.0f) * 5.0f;
            DrawCircleLines(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y - 65), pulse, {0, 170, 255, 100});
        }
    } else {
        Color c = state == PlayerState::Hit ? RED : state == PlayerState::Attack ? YELLOW : BLUE;
        DrawRectangle(static_cast<int>(screenPos.x - 20), static_cast<int>(screenPos.y - 80), 40, 80, c);
        const int eyeX = static_cast<int>(facing == Facing::Right ? screenPos.x + 10 : screenPos.x - 20);
        DrawRectangle(eyeX, static_cast<int>(screenPos.y - 70), 10, 10, SKYBLUE);
    }
}

} // namespace district_fury
