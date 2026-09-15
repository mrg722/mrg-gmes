#pragma once
#include "game/Types.h"
#include "rendering/Animator.h"
#include "ui/SpanishText.h"

namespace district_fury {

enum class PlayerState { Idle, Walk, Dash, Attack, Block, Hit, GuardBreak, Defeat };
enum class AttackType { None, Punch, Kick, Energy };

class Player {
public:
    Vector3D position;
    Vector3D velocity;
    Facing facing;
    PlayerState state;
    AttackType attackType;
    int hp; int maxHp; int sp; int maxSp; int rage; int maxRage; bool isRageMode;
    int shield; int maxShield;
    float stateTimer; float attackElapsed; float attackDuration; float dashTimer; float dashInvulnerability; float comboWindow; float spRegenAccumulator; float rageDrainAccumulator; float blockDamageReduction; float blockTimer; float shieldRegenTimer; float shieldRegenRate;
    int comboCount; int comboStep; bool hasHit; bool energyReleased;
    Animator animator;
    Player();
    void Update(float dt); void Draw() const; void TakeDamage(int damage); void SetState(PlayerState newState); void Reset(); void AddRage(int amount);
    bool AttackIsActive() const; bool IsBlocking() const; bool IsGuardBroken() const; int GetAttackDamage() const; float GetAttackRange() const; float GetAttackDepthRange() const; float GetAttackKnockback() const; CombatBox GetHurtbox() const; CombatBox GetAttackHitbox() const;
};
}
