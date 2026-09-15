#include "game/Player.h"
#include "rendering/AssetManager.h"
#include "audio/AudioSystem.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {

float DepthScale(float laneY) {
    const float t = std::clamp((laneY - kLaneMinY) / (kLaneMaxY - kLaneMinY), 0.0f, 1.0f);
    return 0.86f + 0.26f * t;
}

}

Player::Player() { Reset(); }

void Player::Reset() {
    position = {180.0f, 565.0f, 0.0f};
    velocity = {0.0f, 0.0f, 0.0f};
    facing = Facing::Right;
    state = PlayerState::Idle;
    attackType = AttackType::None;
    maxHp = 100; hp = maxHp;
    maxSp = 100; sp = maxSp;
    maxRage = 100; rage = 0; isRageMode = false;
    stateTimer = 0.0f; attackElapsed = 0.0f; attackDuration = 0.0f;
    dashTimer = 0.0f; dashInvulnerability = 0.0f;
    comboWindow = 0.0f; spRegenAccumulator = 0.0f; rageDrainAccumulator = 0.0f;
    blockDamageReduction = 0.78f; blockTimer = 0.0f;
    comboCount = 0; comboStep = 0; hasHit = false; energyReleased = false;
    animator = Animator{};
}

void Player::SetState(PlayerState newState) {
    if (state == newState && newState != PlayerState::Attack) return;
    state = newState;
    hasHit = false;
    switch (state) {
        case PlayerState::Idle:
            animator.Play(animator.normalizedAtlas ? AnimationClip{0, 3, 0.12f, true} : AnimationClip{0, 4, 0.12f, true}); break;
        case PlayerState::Walk:
            animator.Play(animator.normalizedAtlas ? AnimationClip{4, 7, 0.105f, true} : AnimationClip{0, 4, 0.10f, true}); break;
        case PlayerState::Dash:
            animator.Play(animator.normalizedAtlas ? AnimationClip{14, 14, 0.08f, false} : AnimationClip{0, 4, 0.08f, false}); break;
        case PlayerState::Block:
            animator.Play(animator.normalizedAtlas ? AnimationClip{3, 3, 0.10f, true} : AnimationClip{0, 4, 0.10f, true});
            blockTimer = 0.0f; break;
        case PlayerState::Hit:
            animator.Play(animator.normalizedAtlas ? AnimationClip{13, 13, 0.08f, false} : AnimationClip{0, 4, 0.08f, false});
            stateTimer = 0.17f; break;
        case PlayerState::Defeat:
            animator.Play(animator.normalizedAtlas ? AnimationClip{15, 15, 0.10f, false} : AnimationClip{14, 14, 0.10f, false}); break;
        case PlayerState::Attack: break;
    }
}

static void EnsurePlayerAnimator(Animator& animator) {
    if (animator.texture.id != 0) return;
    Texture2D clean = AssetManager::Get().GetTexture("rayden_clean");
    if (clean.id != 0) { animator.Init(clean, 4, 4, true); animator.Play({0, 3, 0.12f, true}); return; }
    Texture2D legacy = AssetManager::Get().GetTexture("rayden_sheet");
    if (legacy.id != 0) { animator.Init(legacy, 5, 3, false); animator.Play({0, 4, 0.12f, true}); }
}

void Player::Update(float dt) {
    EnsurePlayerAnimator(animator);
    animator.Update(dt);
    dashInvulnerability = std::max(0.0f, dashInvulnerability - dt);

    if (comboWindow > 0.0f) { comboWindow -= dt; if (comboWindow <= 0.0f) { comboWindow = 0.0f; comboCount = 0; comboStep = 0; } }

    if (isRageMode) {
        rageDrainAccumulator += dt;
        if (rageDrainAccumulator >= 0.05f) { const int drain = std::max(1, static_cast<int>(rageDrainAccumulator * 20.0f)); rage = std::max(0, rage - drain); rageDrainAccumulator = 0.0f; }
        if (rage <= 0) { rage = 0; isRageMode = false; }
    }

    if (state == PlayerState::Defeat) return;

    // Hitstun is intentionally short. A separate post-hit grace window prevents enemy chains.
    if (state == PlayerState::Hit) {
        stateTimer -= dt;
        position.x += velocity.x * dt; position.y += velocity.y * dt;
        velocity.x *= 0.80f; velocity.y *= 0.80f;
        if (stateTimer <= 0.0f) SetState(PlayerState::Idle);
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.0f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        return;
    }

    if (state == PlayerState::Dash) {
        dashTimer -= dt; const float direction = facing == Facing::Right ? 1.0f : -1.0f;
        position.x += direction * 700.0f * dt;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.0f);
        if (dashTimer <= 0.0f) SetState(PlayerState::Idle);
        return;
    }

    if (state == PlayerState::Attack) {
        attackElapsed += dt;
        stateTimer -= dt;
        if (attackType == AttackType::Energy && !energyReleased && attackElapsed >= 0.20f) energyReleased = true;
        if (stateTimer <= 0.0f || animator.isFinished) { attackType = AttackType::None; SetState(PlayerState::Idle); }
        return;
    }

    // B is a hold-to-block command. It takes priority over movement and attacks.
    if (IsKeyDown(KEY_B)) {
        attackType = AttackType::None;
        if (state != PlayerState::Block) SetState(PlayerState::Block);
        blockTimer += dt;
        rageDrainAccumulator += dt;
        return;
    }
    if (state == PlayerState::Block) SetState(PlayerState::Idle);

    Vector2 moveDir = {0.0f, 0.0f};
    if (IsKeyDown(KEY_W)) moveDir.y -= 1.0f;
    if (IsKeyDown(KEY_S)) moveDir.y += 1.0f;
    if (IsKeyDown(KEY_A)) moveDir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveDir.x += 1.0f;
    const bool moving = moveDir.x != 0.0f || moveDir.y != 0.0f;
    const float baseSpeed = isRageMode ? 285.0f : 245.0f;

    if (moving) {
        const float len = std::sqrt(moveDir.x * moveDir.x + moveDir.y * moveDir.y);
        position.x += (moveDir.x / len) * baseSpeed * dt;
        position.y += (moveDir.y / len) * baseSpeed * 0.70f * dt;
        if (moveDir.x < 0.0f) facing = Facing::Left;
        if (moveDir.x > 0.0f) facing = Facing::Right;
        if (state != PlayerState::Walk) SetState(PlayerState::Walk);
    } else if (state == PlayerState::Walk) SetState(PlayerState::Idle);

    if (IsKeyPressed(KEY_LEFT_SHIFT)) { dashTimer = 0.12f; dashInvulnerability = 0.20f; SetState(PlayerState::Dash); AudioSystem::Get().Play(Sfx::Dash); return; }

    auto beginAttack = [&](AttackType type, float duration, int cleanStart, int cleanEnd, int legacyStart, int legacyEnd, float frameDuration) {
        state = PlayerState::Attack; attackType = type; attackElapsed = 0.0f; attackDuration = duration; stateTimer = duration; hasHit = false; energyReleased = false;
        const int startFrame = animator.normalizedAtlas ? cleanStart : legacyStart;
        const int endFrame = animator.normalizedAtlas ? cleanEnd : legacyEnd;
        animator.Play({startFrame, endFrame, frameDuration, false});
    };

    if (IsKeyPressed(KEY_J)) { comboStep = (comboWindow > 0.0f) ? (comboStep + 1) % 3 : 0; beginAttack(AttackType::Punch, 0.30f, 8, 9, 5, 9, 0.15f); AudioSystem::Get().Play(Sfx::Punch); return; }
    if (IsKeyPressed(KEY_K)) { comboStep = 3; beginAttack(AttackType::Kick, 0.36f, 10, 11, 10, 14, 0.16f); AudioSystem::Get().Play(Sfx::Kick); return; }
    if (IsKeyPressed(KEY_L) && sp >= 20) { sp -= 20; beginAttack(AttackType::Energy, 0.52f, 12, 13, 5, 9, 0.13f); AudioSystem::Get().Play(Sfx::EnergyCharge); return; }
    if (IsKeyPressed(KEY_SPACE) && rage >= maxRage && !isRageMode) { isRageMode = true; rageDrainAccumulator = 0.0f; AudioSystem::Get().Play(Sfx::Rage); return; }

    spRegenAccumulator += dt;
    while (spRegenAccumulator >= 0.2f && sp < maxSp) { ++sp; spRegenAccumulator -= 0.2f; }
}

void Player::AddRage(int amount) { if (amount <= 0 || isRageMode) return; rage = std::min(maxRage, rage + amount); }
bool Player::IsBlocking() const { return state == PlayerState::Block; }

bool Player::AttackIsActive() const {
    if (state != PlayerState::Attack) return false;
    if (attackType == AttackType::Punch) return attackElapsed >= 0.09f && attackElapsed <= 0.22f;
    if (attackType == AttackType::Kick) return attackElapsed >= 0.11f && attackElapsed <= 0.28f;
    if (attackType == AttackType::Energy) return attackElapsed >= 0.20f && attackElapsed <= 0.43f;
    return false;
}

int Player::GetAttackDamage() const { int damage = 10; if (attackType == AttackType::Kick) damage = 14; if (attackType == AttackType::Energy) damage = 18; if (comboStep >= 2 && attackType == AttackType::Punch) damage += 4; if (isRageMode) damage = static_cast<int>(std::round(damage * 1.35f)); return damage; }
float Player::GetAttackRange() const { if (attackType == AttackType::Kick) return 135.0f; if (attackType == AttackType::Energy) return 220.0f; return 115.0f; }
float Player::GetAttackDepthRange() const { return attackType == AttackType::Energy ? 55.0f : 42.0f; }
float Player::GetAttackKnockback() const { if (attackType == AttackType::Kick) return 420.0f; if (attackType == AttackType::Energy) return 500.0f; return 300.0f; }
CombatBox Player::GetHurtbox() const { return {position.x - 24.0f, position.y - 112.0f, 48.0f, 105.0f}; }
CombatBox Player::GetAttackHitbox() const {
    if (!AttackIsActive()) return {};
    const float direction = facing == Facing::Right ? 1.0f : -1.0f;
    float width = 82.0f, height = 48.0f, forward = 54.0f, centerY = position.y - 76.0f;
    if (attackType == AttackType::Kick) { width = 122.0f; height = 58.0f; forward = 66.0f; centerY = position.y - 64.0f; }
    else if (attackType == AttackType::Energy) { width = 220.0f; height = 76.0f; forward = 122.0f; centerY = position.y - 72.0f; }
    const float centerX = position.x + direction * forward;
    return {centerX - width * 0.5f, centerY - height * 0.5f, width, height};
}

void Player::TakeDamage(int damage) {
    if (state == PlayerState::Defeat || dashInvulnerability > 0.0f) return;
    if (IsBlocking()) {
        const int reduced = std::max(1, static_cast<int>(std::round(damage * (1.0f - blockDamageReduction))));
        hp = std::max(1, hp - reduced);
        AddRage(12);
        velocity.x += (facing == Facing::Right ? -1.0f : 1.0f) * 55.0f;
        return;
    }
    if (state == PlayerState::Hit && stateTimer > 0.0f) return;
    hp = std::max(0, hp - damage);
    AddRage(15);
    if (hp == 0) SetState(PlayerState::Defeat); else { SetState(PlayerState::Hit); dashInvulnerability = 0.28f; }
}

void Player::Draw() const {
    const Vector2 screenPos = position.ToScreen();
    const float depthScale = DepthScale(position.y);
    const float visualScale = animator.normalizedAtlas ? 1.10f * depthScale : 0.76f * depthScale;
    DrawEllipse(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y), 30.0f * depthScale, 8.0f * depthScale, {0,0,0,145});
    if (animator.texture.id != 0) {
        Color tint = WHITE;
        if (state == PlayerState::Hit) tint = {255,190,190,255};
        if (state == PlayerState::Block) tint = {175,220,255,255};
        if (isRageMode) tint = {190,220,255,255};
        animator.Draw(screenPos, visualScale, facing == Facing::Left, tint);
        if (IsBlocking()) {
            const float pulse = 43.0f + std::sin(static_cast<float>(GetTime()) * 12.0f) * 4.0f;
            DrawCircleLines(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y - 70.0f * depthScale), pulse * depthScale, {80,190,255,160});
            DrawText("BLOQUEO", static_cast<int>(screenPos.x - 35), static_cast<int>(screenPos.y - 150), 12, {120,215,255,230});
        }
        if (isRageMode) {
            const float pulse = (38.0f + std::sin(static_cast<float>(GetTime()) * 10.0f) * 6.0f) * depthScale;
            DrawCircleLines(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y - 64.0f * depthScale), pulse, {0,170,255,120});
            DrawCircleLines(static_cast<int>(screenPos.x), static_cast<int>(screenPos.y - 64.0f * depthScale), pulse * 0.72f, {120,230,255,90});
        }
        if (state == PlayerState::Attack && attackType == AttackType::Energy) {
            const float charge = std::clamp(attackElapsed / 0.20f, 0.0f, 1.0f);
            DrawCircleLines(static_cast<int>(screenPos.x + (facing == Facing::Right ? 28 : -28)), static_cast<int>(screenPos.y - 75), 12 + 18 * charge, {50,210,255,120});
            if (energyReleased) DrawCircle(static_cast<int>(screenPos.x + (facing == Facing::Right ? 45 : -45)), static_cast<int>(screenPos.y - 76), 6, {120,240,255,230});
        }
    } else {
        Color c = state == PlayerState::Hit ? RED : state == PlayerState::Attack ? YELLOW : state == PlayerState::Block ? {80,190,255,255} : BLUE;
        DrawRectangle(static_cast<int>(screenPos.x - 18 * depthScale), static_cast<int>(screenPos.y - 68 * depthScale), static_cast<int>(36 * depthScale), static_cast<int>(68 * depthScale), c);
    }
}

} // namespace district_fury
