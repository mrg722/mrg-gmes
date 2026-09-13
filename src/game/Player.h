#pragma once
#include "game/Types.h"

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
    
    float stateTimer;
    int comboCount;
    bool hasHit;

    Player();
    
    void Update(float dt);
    void Draw() const;
    void TakeDamage(int damage);
};

}
