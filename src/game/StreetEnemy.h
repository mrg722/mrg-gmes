#pragma once
#include "game/Types.h"
#include "game/Player.h"
#include "rendering/Animator.h"

namespace district_fury {

enum class StreetEnemyState {
    Idle,
    Chase,
    Attack,
    Hit,
    Defeat
};

enum class StreetEnemyType {
    Punk,
    Brute,
    Charger,
    Enforcer
};

class StreetEnemy {
public:
    Vector3D position;
    Vector3D velocity;
    Facing facing;
    StreetEnemyState state;
    StreetEnemyType type;
    bool active;

    int hp;
    int maxHp;
    float moveSpeed;
    float attackDamage;
    float attackRange;
    float attackDepth;
    float attackDuration;
    float stateTimer;
    float attackElapsed;
    bool hasHit;
    Animator animator;

    StreetEnemy();

    void Init(Vector3D startPos, StreetEnemyType enemyType);
    void Activate();
    void Update(float dt, const Player& player);
    void Draw() const;
    void TakeDamage(int damage, Vector3D knockback);

    bool AttackIsActive() const;
    bool IsDefeated() const;
    CombatBox GetHurtbox() const;
    CombatBox GetAttackHitbox() const;
    const char* GetTypeName() const;
};

}
