#include "game/Player.h"
#include "rendering/AssetManager.h"
#include "audio/AudioSystem.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace { float DepthScale(float y) { return 0.86f + 0.26f * std::clamp((y - kLaneMinY) / (kLaneMaxY - kLaneMinY), 0.0f, 1.0f); } }

Player::Player() { Reset(); }

void Player::Reset() {
	position = {180, 565, 0}; velocity = {0, 0, 0}; facing = Facing::Right;
	state = PlayerState::Idle; attackType = AttackType::None;
	maxHp = 100; hp = maxHp; maxSp = 100; sp = maxSp; maxRage = 100; rage = 0;
	isRageMode = false; maxShield = 100; shield = maxShield; shieldRegenTimer = 0; shieldRegenRate = 34.0f;
	stateTimer = 0; attackElapsed = 0; attackDuration = 0; dashTimer = 0; dashInvulnerability = 0;
	comboWindow = 0; spRegenAccumulator = 0; rageDrainAccumulator = 0; blockDamageReduction = .78f;
	blockTimer = 0; comboCount = 0; comboStep = 0; hasHit = false; energyReleased = false; animator = Animator{};
}

void Player::SetState(PlayerState next) {
	if (state == next && next != PlayerState::Attack) return;
	state = next; hasHit = false;
	switch (state) {
		case PlayerState::Idle: animator.Play(animator.normalizedAtlas ? AnimationClip{0,3,.12f,true} : AnimationClip{0,4,.12f,true}); break;
		case PlayerState::Walk: animator.Play(animator.normalizedAtlas ? AnimationClip{4,7,.105f,true} : AnimationClip{0,4,.10f,true}); break;
		case PlayerState::Dash: animator.Play(animator.normalizedAtlas ? AnimationClip{14,14,.08f,false} : AnimationClip{0,4,.08f,false}); break;
		case PlayerState::Block: animator.Play(animator.normalizedAtlas ? AnimationClip{3,3,.10f,true} : AnimationClip{0,4,.10f,true}); blockTimer = 0; break;
		case PlayerState::Hit: animator.Play(animator.normalizedAtlas ? AnimationClip{13,13,.08f,false} : AnimationClip{0,4,.08f,false}); stateTimer = .13f; break;
		case PlayerState::GuardBreak: animator.Play(animator.normalizedAtlas ? AnimationClip{13,13,.08f,false} : AnimationClip{0,4,.08f,false}); stateTimer = .42f; break;
		case PlayerState::Defeat: animator.Play(animator.normalizedAtlas ? AnimationClip{15,15,.10f,false} : AnimationClip{14,14,.10f,false}); break;
		case PlayerState::Attack: break;
	}
}

static void EnsurePlayerAnimator(Animator& animator) {
	if (animator.texture.id != 0) return;
	Texture2D clean = AssetManager::Get().GetTexture("rayden_clean");
	if (clean.id != 0) { animator.Init(clean, 4, 4, true); animator.Play({0,3,.12f,true}); return; }
	Texture2D legacy = AssetManager::Get().GetTexture("rayden_sheet");
	if (legacy.id != 0) { animator.Init(legacy, 5, 3, false); animator.Play({0,4,.12f,true}); }
}

void Player::Update(float dt) {
	EnsurePlayerAnimator(animator); animator.Update(dt);
	dashInvulnerability = std::max(0.0f, dashInvulnerability - dt);
	if (shield < maxShield && !IsBlocking() && state != PlayerState::Hit && state != PlayerState::GuardBreak) {
		shieldRegenTimer -= dt;
		if (shieldRegenTimer <= 0) shield = std::min(maxShield, shield + (int)std::ceil(shieldRegenRate * dt));
	}
	if (comboWindow > 0 && (comboWindow -= dt) <= 0) { comboWindow = 0; comboCount = 0; comboStep = 0; }
	if (isRageMode) { rageDrainAccumulator += dt; if (rageDrainAccumulator >= .05f) { rage = std::max(0, rage - std::max(1, (int)(rageDrainAccumulator * 20))); rageDrainAccumulator = 0; } if (rage == 0) isRageMode = false; }
	if (state == PlayerState::Defeat) return;
	if (state == PlayerState::GuardBreak) { if ((stateTimer -= dt) <= 0) SetState(PlayerState::Idle); return; }
	if (state == PlayerState::Hit) { stateTimer -= dt; position.x += velocity.x * dt; position.y += velocity.y * dt; velocity.x *= .80f; velocity.y *= .80f; if (stateTimer <= 0) SetState(PlayerState::Idle); position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f); position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY); return; }
	if (state == PlayerState::Dash) { dashTimer -= dt; position.x += (facing == Facing::Right ? 1.f : -1.f) * 700.f * dt; position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.f); if (dashTimer <= 0) SetState(PlayerState::Idle); return; }
	if (state == PlayerState::Attack) { attackElapsed += dt; stateTimer -= dt; if (attackType == AttackType::Energy && !energyReleased && attackElapsed >= .20f) energyReleased = true; if (stateTimer <= 0 || animator.isFinished) { attackType = AttackType::None; SetState(PlayerState::Idle); } return; }
	if (IsKeyDown(KEY_B)) { attackType = AttackType::None; if (state != PlayerState::Block) SetState(PlayerState::Block); blockTimer += dt; return; }
	if (state == PlayerState::Block) SetState(PlayerState::Idle);
	Vector2 move = {0, 0}; if (IsKeyDown(KEY_W)) move.y -= 1; if (IsKeyDown(KEY_S)) move.y += 1; if (IsKeyDown(KEY_A)) move.x -= 1; if (IsKeyDown(KEY_D)) move.x += 1;
	bool moving = move.x != 0 || move.y != 0; float speed = isRageMode ? 285.f : 245.f;
	if (moving) { float length = std::sqrt(move.x * move.x + move.y * move.y); position.x += move.x / length * speed * dt; position.y += move.y / length * speed * .70f * dt; if (move.x < 0) facing = Facing::Left; if (move.x > 0) facing = Facing::Right; if (state != PlayerState::Walk) SetState(PlayerState::Walk); }
	else if (state == PlayerState::Walk) SetState(PlayerState::Idle);
	if (IsKeyPressed(KEY_LEFT_SHIFT)) { dashTimer = .12f; dashInvulnerability = .20f; SetState(PlayerState::Dash); AudioSystem::Get().Play(Sfx::Dash); return; }
	auto beginAttack = [&](AttackType type, float duration, int cleanStart, int cleanEnd, int legacyStart, int legacyEnd, float frameDuration) { state = PlayerState::Attack; attackType = type; attackElapsed = 0; attackDuration = duration; stateTimer = duration; hasHit = false; energyReleased = false; int start = animator.normalizedAtlas ? cleanStart : legacyStart; int end = animator.normalizedAtlas ? cleanEnd : legacyEnd; animator.Play({start, end, frameDuration, false}); };
	if (IsKeyPressed(KEY_J)) { comboStep = comboWindow > 0 ? (comboStep + 1) % 3 : 0; beginAttack(AttackType::Punch, .30f, 8, 9, 5, 9, .15f); AudioSystem::Get().Play(Sfx::Punch); return; }
	if (IsKeyPressed(KEY_K)) { comboStep = 3; beginAttack(AttackType::Kick, .36f, 10, 11, 10, 14, .16f); AudioSystem::Get().Play(Sfx::Kick); return; }
	if (IsKeyPressed(KEY_L) && sp >= 20) { sp -= 20; beginAttack(AttackType::Energy, .52f, 12, 13, 5, 9, .13f); AudioSystem::Get().Play(Sfx::EnergyCharge); return; }
	if (IsKeyPressed(KEY_SPACE) && rage >= maxRage && !isRageMode) { isRageMode = true; rageDrainAccumulator = 0; AudioSystem::Get().Play(Sfx::Rage); return; }
	spRegenAccumulator += dt; while (spRegenAccumulator >= .2f && sp < maxSp) { ++sp; spRegenAccumulator -= .2f; }
}

void Player::AddRage(int amount) { if (amount > 0 && !isRageMode) rage = std::min(maxRage, rage + amount); }
bool Player::IsBlocking() const { return state == PlayerState::Block; }
bool Player::IsGuardBroken() const { return state == PlayerState::GuardBreak; }
bool Player::AttackIsActive() const { if (state != PlayerState::Attack) return false; if (attackType == AttackType::Punch) return attackElapsed >= .09f && attackElapsed <= .22f; if (attackType == AttackType::Kick) return attackElapsed >= .11f && attackElapsed <= .28f; return attackType == AttackType::Energy && attackElapsed >= .20f && attackElapsed <= .43f; }
int Player::GetAttackDamage() const { int damage = attackType == AttackType::Kick ? 14 : attackType == AttackType::Energy ? 18 : 10; if (comboStep >= 2 && attackType == AttackType::Punch) damage += 4; return isRageMode ? (int)std::round(damage * 1.35f) : damage; }
float Player::GetAttackRange() const { return attackType == AttackType::Kick ? 135.f : attackType == AttackType::Energy ? 220.f : 115.f; }
float Player::GetAttackDepthRange() const { return attackType == AttackType::Energy ? 55.f : 42.f; }
float Player::GetAttackKnockback() const { return attackType == AttackType::Kick ? 420.f : attackType == AttackType::Energy ? 500.f : 300.f; }
CombatBox Player::GetHurtbox() const { return {position.x - 24.f, position.y - 112.f, 48.f, 105.f}; }
CombatBox Player::GetAttackHitbox() const { if (!AttackIsActive()) return {}; float direction = facing == Facing::Right ? 1.f : -1.f; float width = 82.f, height = 48.f, forward = 54.f, centerY = position.y - 76.f; if (attackType == AttackType::Kick) { width = 122.f; height = 58.f; forward = 66.f; centerY = position.y - 64.f; } else if (attackType == AttackType::Energy) { width = 220.f; height = 76.f; forward = 122.f; centerY = position.y - 72.f; } float centerX = position.x + direction * forward; return {centerX - width * .5f, centerY - height * .5f, width, height}; }
void Player::TakeDamage(int damage) { if (damage <= 0 || state == PlayerState::Defeat || dashInvulnerability > 0) return; shieldRegenTimer = 1.25f; if (IsBlocking()) { int reduced = std::max(1, (int)std::round(damage * (1.f - blockDamageReduction))); int absorbed = std::min(shield, reduced); shield -= absorbed; hp = std::max(0, hp - reduced + absorbed); AddRage(12); velocity.x += facing == Facing::Right ? -55.f : 55.f; dashInvulnerability = .12f; if (hp == 0) SetState(PlayerState::Defeat); else if (shield == 0) SetState(PlayerState::GuardBreak); return; } if (state == PlayerState::Hit && stateTimer > 0) return; hp = std::max(0, hp - damage); AddRage(15); if (hp == 0) SetState(PlayerState::Defeat); else { SetState(PlayerState::Hit); dashInvulnerability = .28f; } }
void Player::Draw() const { Vector2 p = position.ToScreen(); float scale = DepthScale(position.y); float spriteScale = animator.normalizedAtlas ? 1.10f * scale : .76f * scale; DrawEllipse((int)p.x, (int)p.y, 30 * scale, 8 * scale, {0,0,0,145}); if (IsBlocking() || shield < maxShield) { float ratio = std::clamp((float)shield / maxShield, 0.f, 1.f); float pulse = (1.f - ratio) * 3.f + std::sin((float)GetTime() * 10.f) * (ratio < .3f ? 2.f : .5f); Color shieldColor = ratio < .3f ? Color{255,70,70,34} : Color{45,180,255,28}; DrawEllipse((int)p.x, (int)(p.y - 63 * scale), (38 + pulse) * scale, (76 + pulse) * scale, shieldColor); DrawEllipseLines((int)p.x, (int)(p.y - 63 * scale), (38 + pulse) * scale, (76 + pulse) * scale, ratio < .3f ? Color{255,90,80,230} : Color{80,215,255,210}); DrawEllipseLines((int)p.x, (int)(p.y - 63 * scale), (33 + pulse) * scale, (70 + pulse) * scale, {180,245,255,125}); } if (animator.texture.id != 0) { Color tint = state == PlayerState::Hit ? Color{255,190,190,255} : state == PlayerState::Block ? Color{175,220,255,255} : isRageMode ? Color{190,220,255,255} : WHITE; animator.Draw(p, spriteScale, facing == Facing::Left, tint); } else { Color tint = state == PlayerState::Hit ? RED : state == PlayerState::Attack ? YELLOW : state == PlayerState::Block ? Color{80,190,255,255} : BLUE; DrawRectangle((int)(p.x - 18 * scale), (int)(p.y - 68 * scale), (int)(36 * scale), (int)(68 * scale), tint); } if (IsBlocking()) { const char* label = shield == 0 ? "ESCUDO ROTO" : "BLOQUEO"; DrawText(label, (int)p.x - MeasureText(label, 12) / 2, (int)p.y - 154, 12, {120,215,255,230}); } if (state == PlayerState::Attack && attackType == AttackType::Energy) { float charge = std::clamp(attackElapsed / .20f, 0.f, 1.f); DrawCircleLines((int)(p.x + (facing == Facing::Right ? 28 : -28)), (int)(p.y - 75), 12 + 18 * charge, {50,210,255,120}); } }
}
