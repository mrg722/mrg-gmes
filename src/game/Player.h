#pragma once
#include "game/Types.h"
#include "rendering/Animator.h"

namespace district_fury {

enum class PlayerState {
    Idle,
    Walk,
    Dash,
    Attack,
    Hit,
    Defeat
};

enum class AttackType {
    None,
    Punch,
    Kick,
    Energy
};

class Player {
public:
    Vector3D position;
    Vector3D velocity;
    Facing facing;
    PlayerState state;
    AttackType attackType;

    int hp;
    int maxHp;
    int sp;
    int maxSp;
    int rage;
    int maxRage;
    bool isRageMode;

    float stateTimer;
    float attackElapsed;
    float attackDuration;
    float dashTimer;
    float dashInvulnerability;
    float comboWindow;
    float spRegenAccumulator;
    float rageDrainAccumulator;
    int comboCount;
    int comboStep;
    bool hasHit;

    Animator animator;

    Player();

    void Update(float dt);
    void Draw() const;
    void TakeDamage(int damage);
    void SetState(PlayerState newState);
    void Reset();
    void AddRage(int amount);

    bool AttackIsActive() const;
    int GetAttackDamage() const;
    float GetAttackRange() const;
    float GetAttackDepthRange() const;
    float GetAttackKnockback() const;
    CombatBox GetHurtbox() const;
    CombatBox GetAttackHitbox() const;
};

}
