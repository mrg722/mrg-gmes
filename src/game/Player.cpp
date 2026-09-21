#include "game/Player.h"
#include "rendering/AssetManager.h"
#include "rendering/BossSprite.h"
#include "game/CharacterVisual.h"
#include <string>
#include "audio/AudioSystem.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {

constexpr float kRageStartDuration = 0.45f;
constexpr float kRageEndDuration = 0.55f;
constexpr float kRageMaxDuration = 9.0f;

AttackType LegacyType(AttackId id) {
    switch (id) {
        case AttackId::Punch1:
        case AttackId::Punch2:
        case AttackId::Punch3:     return AttackType::Punch;
        case AttackId::Kick:       return AttackType::Kick;
        case AttackId::EnergyWave: return AttackType::Energy;
        case AttackId::DashAttack: return AttackType::Dash;
        case AttackId::RageAttack: return AttackType::Rage;
        case AttackId::Finisher:   return AttackType::Finisher;
        default:                   return AttackType::None;
    }
}

struct ClipRange { int cleanStart; int cleanEnd; int legacyStart; int legacyEnd; float frameTime; };

ClipRange ClipFor(AttackId id) {
    switch (id) {
        case AttackId::Punch1:     return {8, 9, 5, 9, 0.150f};
        case AttackId::Punch2:     return {8, 9, 5, 9, 0.135f};
        case AttackId::Punch3:     return {10, 11, 10, 14, 0.150f};
        case AttackId::Kick:       return {10, 11, 10, 14, 0.160f};
        case AttackId::EnergyWave: return {12, 13, 5, 9, 0.130f};
        case AttackId::DashAttack: return {8, 9, 5, 9, 0.120f};
        case AttackId::RageAttack: return {10, 11, 10, 14, 0.140f};
        case AttackId::Finisher:   return {12, 13, 10, 14, 0.150f};
        default:                   return {0, 3, 0, 4, 0.120f};
    }
}

}  // namespace

Player::Player() { Reset(); }

void Player::Reset() {
    position = {180, 565, 0};
    velocity = {0, 0, 0};
    facing = Facing::Right;
    state = PlayerState::Idle;
    attackType = AttackType::None;
    currentAttack = AttackId::None;

    maxHp = 100 + upgrades.bonusMaxHp;
    hp = maxHp;
    maxSp = 100 + upgrades.bonusMaxSp;
    sp = maxSp;
    maxRage = static_cast<int>(std::round(100.0f * upgrades.rageCapacityScale));
    rage = 0;
    isRageMode = false;
    maxShield = 100 + upgrades.bonusMaxShield;
    shield = maxShield;
    shieldRegenTimer = 0;
    shieldRegenRate = 34.0f;

    stateTimer = 0; attackElapsed = 0; attackDuration = 0;
    dashTimer = 0; dashInvulnerability = 0; dashCooldown = 0;
    comboWindow = 0; spRegenAccumulator = 0; rageDrainAccumulator = 0;
    blockDamageReduction = 0.78f; blockTimer = 0;

    rageState = RageState::Normal;
    rageStateTimer = 0;
    rageAuraPhase = 0;

    damageBuffTimer = 0;
    speedBuffTimer = 0;
    hitstunTimer = 0;
    knockdownTimer = 0;

    for (int i = 0; i < static_cast<int>(AttackId::Count); ++i) attackCooldowns[i] = 0.0f;

    comboCount = 0; comboStep = 0; hasHit = false; energyReleased = false;
    inputEnabled = true;
    debugInvulnerable = false;
    animator = Animator{};
}

void Player::ApplyUpgrades(const PlayerUpgrades& newUpgrades) {
    upgrades = newUpgrades;
    maxHp = 100 + upgrades.bonusMaxHp;
    maxSp = 100 + upgrades.bonusMaxSp;
    maxShield = 100 + upgrades.bonusMaxShield;
    maxRage = static_cast<int>(std::round(100.0f * upgrades.rageCapacityScale));
    hp = std::min(hp <= 0 ? maxHp : hp, maxHp);
    sp = std::min(sp, maxSp);
    shield = std::min(shield, maxShield);
    rage = std::min(rage, maxRage);
}

const AttackDef& Player::CurrentAttackDef() const { return GetAttack(currentAttack); }

const char* Player::RageStateName() const {
    switch (rageState) {
        case RageState::Starting: return "FURIA // ACTIVANDO";
        case RageState::Active:   return "FURIA // ACTIVA";
        case RageState::Ending:   return "FURIA // AGOTANDOSE";
        default:                  return "NORMAL";
    }
}

void Player::SetState(PlayerState next) {
    if (state == next && next != PlayerState::Attack) return;
    state = next;
    hasHit = false;
    const bool normalized = animator.normalizedAtlas;
    switch (state) {
        case PlayerState::Idle:
            animator.Play(normalized ? AnimationClip{0, 3, 0.12f, true}
                                     : AnimationClip{0, 4, 0.12f, true});
            break;
        case PlayerState::Walk:
            animator.Play(normalized ? AnimationClip{4, 7, 0.105f, true}
                                     : AnimationClip{0, 4, 0.10f, true});
            break;
        case PlayerState::Dash:
            animator.Play(normalized ? AnimationClip{14, 14, 0.08f, false}
                                     : AnimationClip{0, 4, 0.08f, false});
            break;
        case PlayerState::Block:
            animator.Play(normalized ? AnimationClip{3, 3, 0.10f, true}
                                     : AnimationClip{0, 4, 0.10f, true});
            blockTimer = 0;
            break;
        case PlayerState::Hit:
            animator.Play(normalized ? AnimationClip{13, 13, 0.08f, false}
                                     : AnimationClip{0, 4, 0.08f, false});
            if (stateTimer <= 0.0f) stateTimer = 0.13f;
            break;
        case PlayerState::GuardBreak:
            animator.Play(normalized ? AnimationClip{13, 13, 0.08f, false}
                                     : AnimationClip{0, 4, 0.08f, false});
            stateTimer = 0.42f;
            break;
        case PlayerState::Knockdown:
            animator.Play(normalized ? AnimationClip{15, 15, 0.10f, false}
                                     : AnimationClip{14, 14, 0.10f, false});
            break;
        case PlayerState::Defeat:
            animator.Play(normalized ? AnimationClip{15, 15, 0.10f, false}
                                     : AnimationClip{14, 14, 0.10f, false});
            AudioSystem::Get().Play(Sfx::GameOver);
            break;
        case PlayerState::Attack:
            break;
    }
}

static void EnsurePlayerAnimator(Animator& animator) {
    if (animator.texture.id != 0) return;
    Texture2D clean = AssetManager::Get().GetTexture("rayden_clean");
    if (clean.id != 0) { animator.Init(clean, 4, 4, true); animator.Play({0, 3, 0.12f, true}); return; }
    Texture2D legacy = AssetManager::Get().GetTexture("rayden_sheet");
    if (legacy.id != 0) { animator.Init(legacy, 5, 3, false); animator.Play({0, 4, 0.12f, true}); }
}

void Player::BeginAttack(AttackId id) {
    const AttackDef& def = GetAttack(id);
    currentAttack = id;
    attackType = LegacyType(id);
    state = PlayerState::Attack;
    attackElapsed = 0;
    attackDuration = AttackTotalDuration(def);
    stateTimer = attackDuration;
    hasHit = false;
    energyReleased = false;
    if (def.cooldown > 0.0f) attackCooldowns[static_cast<int>(id)] = def.cooldown;
    if (def.spCost > 0) sp = std::max(0, sp - def.spCost);

    const ClipRange clip = ClipFor(id);
    const int start = animator.normalizedAtlas ? clip.cleanStart : clip.legacyStart;
    const int end = animator.normalizedAtlas ? clip.cleanEnd : clip.legacyEnd;
    animator.Play({start, end, clip.frameTime, false});

    switch (id) {
        case AttackId::EnergyWave: AudioSystem::Get().Play(Sfx::EnergyCharge); break;
        case AttackId::Kick:
        case AttackId::Finisher:   AudioSystem::Get().Play(Sfx::Kick); break;
        case AttackId::RageAttack: AudioSystem::Get().Play(Sfx::Rage); break;
        default:                   AudioSystem::Get().Play(Sfx::Punch); break;
    }
}

void Player::OnAttackConnected(const AttackDef& def) {
    if (def.canCombo) {
        comboWindow = def.comboWindow + upgrades.comboWindowBonus;
        ++comboCount;
    } else {
        comboWindow = std::max(comboWindow, 0.18f);
    }
    AddRage(def.rageGain);
}

void Player::UpdateRage(float dt) {
    rageAuraPhase += dt;

    switch (rageState) {
        case RageState::Starting:
            rageStateTimer -= dt;
            if (rageStateTimer <= 0.0f) {
                rageState = RageState::Active;
                rageStateTimer = kRageMaxDuration;
                isRageMode = true;
            }
            break;
        case RageState::Active: {
            isRageMode = true;
            rageStateTimer -= dt;
            rageDrainAccumulator += dt;
            if (rageDrainAccumulator >= 0.05f) {
                const int drain = std::max(1, static_cast<int>(rageDrainAccumulator * 11.0f));
                rage = std::max(0, rage - drain);
                rageDrainAccumulator = 0;
            }
            if (rage <= 0 || rageStateTimer <= 0.0f) {
                rageState = RageState::Ending;
                rageStateTimer = kRageEndDuration;
            }
            break;
        }
        case RageState::Ending:
            rageStateTimer -= dt;
            isRageMode = false;
            if (rageStateTimer <= 0.0f) {
                rageState = RageState::Normal;
                rage = 0;
            }
            break;
        case RageState::Normal:
        default:
            isRageMode = false;
            break;
    }
}

bool Player::CanAct() const {
    return state != PlayerState::Defeat && state != PlayerState::GuardBreak &&
           state != PlayerState::Knockdown && state != PlayerState::Hit &&
           state != PlayerState::Attack;
}

bool Player::IsKnockedDown() const { return state == PlayerState::Knockdown; }

bool Player::IsInvulnerable() const {
    return debugInvulnerable || dashInvulnerability > 0.0f;
}

float Player::GetMoveSpeed() const {
    float speed = 245.0f * upgrades.moveSpeedScale;
    if (isRageMode) speed *= 1.16f;
    if (speedBuffTimer > 0.0f) speed *= 1.22f;
    return speed;
}

void Player::HandleInput(float dt) {
    if (!inputEnabled) return;

    if (IsKeyDown(KEY_B)) {
        attackType = AttackType::None;
        currentAttack = AttackId::None;
        if (state != PlayerState::Block) SetState(PlayerState::Block);
        blockTimer += dt;
        return;
    }
    if (state == PlayerState::Block) SetState(PlayerState::Idle);

    Vector2 move = {0, 0};
    if (IsKeyDown(KEY_W)) move.y -= 1;
    if (IsKeyDown(KEY_S)) move.y += 1;
    if (IsKeyDown(KEY_A)) move.x -= 1;
    if (IsKeyDown(KEY_D)) move.x += 1;

    const bool moving = move.x != 0 || move.y != 0;
    const float speed = GetMoveSpeed();
    if (moving) {
        const float length = std::sqrt(move.x * move.x + move.y * move.y);
        position.x += move.x / length * speed * dt;
        position.y += move.y / length * speed * 0.70f * dt;
        if (move.x < 0) facing = Facing::Left;
        if (move.x > 0) facing = Facing::Right;
        if (state != PlayerState::Walk) SetState(PlayerState::Walk);
    } else if (state == PlayerState::Walk) {
        SetState(PlayerState::Idle);
    }

    // Los carriles se acotan aqui, no en cada stage. Stage2 no lo hacia (DF-013 D3).
    position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);

    if (IsKeyPressed(KEY_LEFT_SHIFT) && dashCooldown <= 0.0f) {
        dashTimer = 0.12f;
        dashInvulnerability = 0.20f;
        dashCooldown = 0.38f * upgrades.dashCooldownScale;
        SetState(PlayerState::Dash);
        AudioSystem::Get().Play(Sfx::Dash);
        return;
    }

    // FINISHER: remate de Rage con combo alto.
    if (IsKeyPressed(KEY_K) && isRageMode && comboCount >= 4 &&
        sp >= GetAttack(AttackId::Finisher).spCost &&
        attackCooldowns[static_cast<int>(AttackId::Finisher)] <= 0.0f) {
        BeginAttack(AttackId::Finisher);
        return;
    }

    if (IsKeyPressed(KEY_J)) {
        const AttackId next = comboWindow > 0.0f ? NextPunchInChain(currentAttack)
                                                 : AttackId::Punch1;
        comboStep = next == AttackId::Punch1 ? 0 : next == AttackId::Punch2 ? 1 : 2;
        BeginAttack(next);
        return;
    }

    if (IsKeyPressed(KEY_K)) {
        comboStep = 3;
        BeginAttack(AttackId::Kick);
        return;
    }

    if (IsKeyPressed(KEY_L) && sp >= GetAttack(AttackId::EnergyWave).spCost &&
        attackCooldowns[static_cast<int>(AttackId::EnergyWave)] <= 0.0f) {
        BeginAttack(AttackId::EnergyWave);
        return;
    }

    if (IsKeyPressed(KEY_SPACE)) {
        if (rageState == RageState::Normal && rage >= maxRage) {
            rageState = RageState::Starting;
            rageStateTimer = kRageStartDuration;
            rageDrainAccumulator = 0;
            AudioSystem::Get().Play(Sfx::Rage);
            return;
        }
        if (rageState == RageState::Active &&
            attackCooldowns[static_cast<int>(AttackId::RageAttack)] <= 0.0f) {
            BeginAttack(AttackId::RageAttack);
            return;
        }
    }

    spRegenAccumulator += dt;
    while (spRegenAccumulator >= 0.2f && sp < maxSp) { ++sp; spRegenAccumulator -= 0.2f; }
}

void Player::Update(float dt) {
    EnsurePlayerAnimator(animator);
    animator.Update(dt);

    dashInvulnerability = std::max(0.0f, dashInvulnerability - dt);
    dashCooldown = std::max(0.0f, dashCooldown - dt);
    damageBuffTimer = std::max(0.0f, damageBuffTimer - dt);
    speedBuffTimer = std::max(0.0f, speedBuffTimer - dt);
    for (int i = 0; i < static_cast<int>(AttackId::Count); ++i) {
        attackCooldowns[i] = std::max(0.0f, attackCooldowns[i] - dt);
    }

    UpdateRage(dt);

    if (shield < maxShield && !IsBlocking() && state != PlayerState::Hit &&
        state != PlayerState::GuardBreak && state != PlayerState::Knockdown) {
        shieldRegenTimer -= dt;
        if (shieldRegenTimer <= 0) {
            shield = std::min(maxShield, shield + static_cast<int>(std::ceil(shieldRegenRate * dt)));
        }
    }

    if (comboWindow > 0 && (comboWindow -= dt) <= 0) {
        comboWindow = 0;
        comboCount = 0;
        comboStep = 0;
        currentAttack = AttackId::None;
    }

    // Altura (launch/knockdown): gravedad simple sobre position.z.
    if (position.z > 0.0f || velocity.z != 0.0f) {
        velocity.z -= 1500.0f * dt;
        position.z += velocity.z * dt;
        if (position.z <= 0.0f) { position.z = 0.0f; velocity.z = 0.0f; }
    }

    if (state == PlayerState::Defeat) return;

    if (state == PlayerState::Knockdown) {
        knockdownTimer -= dt;
        position.x += velocity.x * dt;
        velocity.x *= 0.86f;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        if (knockdownTimer <= 0 && position.z <= 0.0f) {
            dashInvulnerability = std::max(dashInvulnerability, 0.45f);
            SetState(PlayerState::Idle);
        }
        return;
    }

    if (state == PlayerState::GuardBreak) {
        if ((stateTimer -= dt) <= 0) SetState(PlayerState::Idle);
        return;
    }

    if (state == PlayerState::Hit) {
        stateTimer -= dt;
        hitstunTimer = std::max(0.0f, hitstunTimer - dt);
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        velocity.x *= 0.80f;
        velocity.y *= 0.80f;
        if (stateTimer <= 0) SetState(PlayerState::Idle);
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        return;
    }

    if (state == PlayerState::Dash) {
        dashTimer -= dt;
        position.x += (facing == Facing::Right ? 1.f : -1.f) * 700.f * dt;
        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f);
        // DASH ATTACK: J durante el dash.
        if (inputEnabled && IsKeyPressed(KEY_J) &&
            attackCooldowns[static_cast<int>(AttackId::DashAttack)] <= 0.0f) {
            BeginAttack(AttackId::DashAttack);
            return;
        }
        if (dashTimer <= 0) SetState(PlayerState::Idle);
        return;
    }

    if (state == PlayerState::Attack) {
        attackElapsed += dt;
        stateTimer -= dt;
        const AttackDef& def = GetAttack(currentAttack);
        if (def.spawnsProjectile && !energyReleased && attackElapsed >= def.startup) {
            energyReleased = true;
        }
        if (def.canCancel && hasHit && inputEnabled && IsKeyPressed(KEY_J) &&
            attackElapsed >= def.startup) {
            const AttackId next = NextPunchInChain(currentAttack);
            comboStep = next == AttackId::Punch1 ? 0 : next == AttackId::Punch2 ? 1 : 2;
            BeginAttack(next);
            return;
        }
        if (stateTimer <= 0 || (animator.isFinished && attackElapsed >= attackDuration)) {
            attackType = AttackType::None;
            SetState(PlayerState::Idle);
        }
        return;
    }

    HandleInput(dt);
}

void Player::AddRage(int amount) {
    if (amount > 0 && rageState == RageState::Normal) rage = std::min(maxRage, rage + amount);
}

bool Player::IsBlocking() const { return state == PlayerState::Block; }
bool Player::IsGuardBroken() const { return state == PlayerState::GuardBreak; }

bool Player::AttackIsActive() const {
    if (state != PlayerState::Attack) return false;
    const AttackDef& def = GetAttack(currentAttack);
    return attackElapsed >= def.startup && attackElapsed <= def.startup + def.active;
}

int Player::GetAttackDamage() const {
    const AttackDef& def = GetAttack(currentAttack);
    float damage = static_cast<float>(def.damage);
    damage *= def.spawnsProjectile ? upgrades.energyDamageScale : upgrades.attackDamageScale;
    if (isRageMode) damage *= 1.35f;
    if (damageBuffTimer > 0.0f) damage *= 1.25f;
    return std::max(1, static_cast<int>(std::round(damage)));
}

float Player::GetAttackRange() const { return GetAttack(currentAttack).range; }
float Player::GetAttackDepthRange() const { return GetAttack(currentAttack).depth; }
float Player::GetAttackKnockback() const { return GetAttack(currentAttack).knockback; }

CombatBox Player::GetHurtbox() const {
    if (state == PlayerState::Knockdown) {
        return {position.x - 38.f, position.y - 40.f, 76.f, 36.f};
    }
    return {position.x - 24.f, position.y - 112.f + -position.z, 48.f, 105.f};
}

CombatBox Player::GetAttackHitbox() const {
    if (!AttackIsActive()) return {};
    const AttackDef& def = GetAttack(currentAttack);
    const float direction = facing == Facing::Right ? 1.f : -1.f;
    const float centerX = position.x + direction * def.boxForward;
    const float centerY = position.y + def.boxCenterOffsetY;
    return {centerX - def.boxWidth * 0.5f, centerY - def.boxHeight * 0.5f,
            def.boxWidth, def.boxHeight};
}

void Player::TakeDamage(int damage) {
    if (damage <= 0 || state == PlayerState::Defeat) return;
    if (debugInvulnerable || dashInvulnerability > 0) return;

    shieldRegenTimer = 1.25f;

    if (IsBlocking()) {
        const int reduced = std::max(1, static_cast<int>(std::round(damage * (1.f - blockDamageReduction))));
        const int absorbed = std::min(shield, reduced);
        shield -= absorbed;
        hp = std::max(0, hp - reduced + absorbed);
        AddRage(12);
        velocity.x += facing == Facing::Right ? -55.f : 55.f;
        dashInvulnerability = 0.12f;
        AudioSystem::Get().Play(Sfx::Hit);
        if (hp == 0) SetState(PlayerState::Defeat);
        else if (shield == 0) { SetState(PlayerState::GuardBreak); AudioSystem::Get().Play(Sfx::HeavyHit); }
        return;
    }

    if (state == PlayerState::Hit && stateTimer > 0) return;
    if (state == PlayerState::Knockdown) return;

    hp = std::max(0, hp - damage);
    AddRage(15);

    if (hp == 0) {
        SetState(PlayerState::Defeat);
        return;
    }

    // Golpes fuertes derriban: knockdown real en vez de un unico estado Hit.
    if (damage >= 28) {
        knockdownTimer = 0.85f;
        velocity.x = facing == Facing::Right ? -260.f : 260.f;
        velocity.z = 380.f;
        SetState(PlayerState::Knockdown);
        AudioSystem::Get().Play(Sfx::HeavyHit);
        return;
    }

    stateTimer = 0.13f;
    hitstunTimer = 0.18f;
    SetState(PlayerState::Hit);
    dashInvulnerability = 0.28f;
}

void Player::DrawRageAura(Vector2 screen, float scale) const {
    if (rageState == RageState::Normal) return;

    float intensity = 1.0f;
    if (rageState == RageState::Starting) {
        intensity = 1.0f - std::clamp(rageStateTimer / kRageStartDuration, 0.0f, 1.0f);
    } else if (rageState == RageState::Ending) {
        intensity = std::clamp(rageStateTimer / kRageEndDuration, 0.0f, 1.0f);
    }
    if (intensity <= 0.01f) return;

    const float pulse = std::sin(rageAuraPhase * 9.0f) * 0.5f + 0.5f;
    const float bodyHeight = 112.0f * scale;
    const float bodyWidth = 40.0f * scale;

    // El aura envuelve el cuerpo: se compone de tres elipses apiladas
    // (piernas / torso / cabeza) en vez de un unico circulo plano.
    struct Blob { float offsetY; float rx; float ry; };
    const Blob blobs[3] = {
        {-bodyHeight * 0.16f, bodyWidth * 0.95f, bodyHeight * 0.22f},
        {-bodyHeight * 0.50f, bodyWidth * 1.05f, bodyHeight * 0.33f},
        {-bodyHeight * 0.84f, bodyWidth * 0.72f, bodyHeight * 0.22f},
    };

    for (int layer = 2; layer >= 0; --layer) {
        const float grow = 1.0f + layer * 0.16f + pulse * 0.07f;
        const unsigned char alpha = static_cast<unsigned char>(
            std::clamp((52.0f - layer * 14.0f) * intensity, 0.0f, 255.0f));
        const Color glow = {255, static_cast<unsigned char>(120 + layer * 30),
                            static_cast<unsigned char>(40 + layer * 20), alpha};
        for (const Blob& blob : blobs) {
            DrawEllipse(static_cast<int>(screen.x),
                        static_cast<int>(screen.y + blob.offsetY),
                        blob.rx * grow, blob.ry * grow, glow);
        }
    }

    // Lenguas de energia ascendentes ancladas a la silueta.
    const int tongues = 9;
    for (int i = 0; i < tongues; ++i) {
        const float phase = rageAuraPhase * 4.2f + static_cast<float>(i) * 1.17f;
        const float t = std::fmod(phase, 1.0f);
        const float side = (i % 2 == 0) ? -1.0f : 1.0f;
        const float x = screen.x + side * bodyWidth * (0.45f + 0.42f * std::sin(phase * 1.7f));
        const float y = screen.y - bodyHeight * (0.06f + t * 0.98f);
        const float size = (5.0f - t * 4.0f) * scale * (0.8f + pulse * 0.4f);
        if (size <= 0.4f) continue;
        const unsigned char alpha = static_cast<unsigned char>(
            std::clamp(215.0f * (1.0f - t) * intensity, 0.0f, 255.0f));
        DrawCircle(static_cast<int>(x), static_cast<int>(y), size,
                   {255, static_cast<unsigned char>(170 + 60 * (1.0f - t)), 70, alpha});
    }

    const unsigned char rimAlpha = static_cast<unsigned char>(
        std::clamp(150.0f * intensity, 0.0f, 255.0f));
    DrawEllipseLines(static_cast<int>(screen.x),
                     static_cast<int>(screen.y - bodyHeight * 0.50f),
                     bodyWidth * (1.22f + pulse * 0.08f),
                     bodyHeight * (0.60f + pulse * 0.04f),
                     {255, 205, 110, rimAlpha});
}

void Player::Draw() const {
    Vector2 p = position.ToScreen();
    const float scale = DepthScaleFor(position.y);
    const float spriteScale = animator.normalizedAtlas ? 1.20f * scale : 0.76f * scale;

    DrawEllipse(static_cast<int>(p.x), static_cast<int>(position.y), 30 * scale, 8 * scale,
                {0, 0, 0, 145});

    DrawRageAura(p, scale);

    if (IsBlocking() || shield < maxShield) {
        const float ratio = std::clamp(static_cast<float>(shield) / maxShield, 0.f, 1.f);
        const float pulse = (1.f - ratio) * 3.f +
            std::sin(static_cast<float>(GetTime()) * 10.f) * (ratio < 0.3f ? 2.f : 0.5f);
        const Color shieldColor = ratio < 0.3f ? Color{255, 70, 70, 34} : Color{45, 180, 255, 28};
        DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y - 63 * scale),
                    (38 + pulse) * scale, (76 + pulse) * scale, shieldColor);
        DrawEllipseLines(static_cast<int>(p.x), static_cast<int>(p.y - 63 * scale),
                         (38 + pulse) * scale, (76 + pulse) * scale,
                         ratio < 0.3f ? Color{255, 90, 80, 230} : Color{80, 215, 255, 210});
        DrawEllipseLines(static_cast<int>(p.x), static_cast<int>(p.y - 63 * scale),
                         (33 + pulse) * scale, (70 + pulse) * scale, {180, 245, 255, 125});
    }

    Color spriteTint = WHITE;
    if (state == PlayerState::Hit || state == PlayerState::Knockdown) spriteTint = {255, 190, 190, 255};
    else if (state == PlayerState::Block) spriteTint = {175, 220, 255, 255};
    else if (isRageMode) spriteTint = {255, 214, 188, 255};
    else if (damageBuffTimer > 0.0f) spriteTint = {255, 226, 190, 255};

    // DF-013.2: personajes adicionales (clon y bosses jugables en VS). El
    // Rayden original (skin 0) conserva su atlas y su ruta de dibujo.
    const CharacterVisual& cv = GetCharacterVisual(skin);
    Texture2D altTex{};
    if (skin != 0 && cv.folder != nullptr) {
        const char* pose = CharacterPose(skin, state, attackType, isRageMode, GetTime());
        altTex = AssetManager::Get().GetTexture(std::string(cv.folder) + "_" + pose);
        if (altTex.id == 0) altTex = AssetManager::Get().GetTexture(std::string(cv.folder) + (cv.uniformCanvas ? "_idle" : "_idle1"));
    }
    if (altTex.id != 0) {
        const bool flip = cv.facesRightByDefault ? (facing == Facing::Left) : (facing == Facing::Right);
        if (cv.uniformCanvas) DrawSpriteUniform(altTex, p, cv.scale * scale, flip, spriteTint, cv.footInset);
        else DrawBossPose(altTex, p, cv.targetHeight * scale, flip, spriteTint);
    } else if (animator.texture.id != 0) {
        animator.Draw(p, spriteScale, facing == Facing::Left, spriteTint);
    } else {
        Color tint = state == PlayerState::Hit ? RED
                   : state == PlayerState::Attack ? YELLOW
                   : state == PlayerState::Block ? Color{80, 190, 255, 255} : BLUE;
        DrawRectangle(static_cast<int>(p.x - 18 * scale), static_cast<int>(p.y - 68 * scale),
                      static_cast<int>(36 * scale), static_cast<int>(68 * scale), tint);
    }

    if (IsBlocking()) {
        const char* label = shield == 0 ? "ESCUDO ROTO" : "BLOQUEO";
        DrawText(label, static_cast<int>(p.x) - MeasureText(label, 12) / 2,
                 static_cast<int>(p.y) - 154, 12, {120, 215, 255, 230});
    }

    // Carga visible de la Energy Wave en las manos.
    const AttackDef& def = GetAttack(currentAttack);
    if (state == PlayerState::Attack && def.spawnsProjectile) {
        const float charge = std::clamp(attackElapsed / std::max(0.01f, def.startup), 0.f, 1.f);
        const float handX = p.x + (facing == Facing::Right ? 30 : -30);
        const float handY = p.y - 76 * scale;
        DrawCircle(static_cast<int>(handX), static_cast<int>(handY), 6 + 16 * charge,
                   {50, 210, 255, static_cast<unsigned char>(60 + 120 * charge)});
        DrawCircleLines(static_cast<int>(handX), static_cast<int>(handY), 12 + 20 * charge,
                        {120, 235, 255, 170});
        for (int i = 0; i < 5; ++i) {
            const float angle = rageAuraPhase * 7.0f + static_cast<float>(i) * 1.25f;
            DrawCircle(static_cast<int>(handX + std::cos(angle) * (16 + 14 * charge)),
                       static_cast<int>(handY + std::sin(angle) * (12 + 10 * charge)),
                       2.5f, {180, 245, 255, 200});
        }
    }
}

void Player::ApplyCharacter(int id) {
    skin = id;
    if (id == 0) return;                       // Rayden original: intacto
    const CharacterVisual& cv = GetCharacterVisual(id);
    maxHp = cv.maxHp;
    hp = maxHp;
}

}  // namespace district_fury
