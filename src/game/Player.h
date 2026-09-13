#pragma once
#include "game/Types.h"
#include "rendering/Animator.h"

namespace district_fury {

enum class PlayerState {
    Idle,
    Walk,
    Attack,
    Hit,
    Defeat
};

class Player {
public:
    Vector3D position;
    Vector3D velocity;
    Facing facing;
    PlayerState state;
    
    int hp;
    int maxHp;
    int sp;
    int maxSp;
    int rage;
    int maxRage;
    bool isRageMode;
    
    float stateTimer;
    int comboCount;
    bool hasHit;

    Animator animator;

    Player();
    
    void Update(float dt);
    void Draw() const;
    void TakeDamage(int damage);
    void SetState(PlayerState newState);
};

}
