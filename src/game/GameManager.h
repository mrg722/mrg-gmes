#pragma once
#include "game/Player.h"
#include "game/Scene.h"
#include "game/Enemy.h"
#include <vector>

namespace district_fury {

enum class GameFlowState {
    Start,
    Combat,
    Win,
    GameOver
};

struct DamageText {
    Vector3D position;
    int amount;
    float timer;
    float maxTime;
};

struct Particle {
    Vector3D position;
    Vector2 velocity;
    float timer;
    float maxTime;
    Color color;
};

class GameManager {
public:
    Player player;
    Enemy enemy;
    Scene scene;
    GameFlowState flowState;
    
    std::vector<DamageText> damageTexts;
    std::vector<Particle> particles;
    
    float hitstopTimer;
    float screenshakeTimer;
    
    GameManager();
    void Init();
    void Update(float dt);
    void Draw() const;
    
    void SpawnDamageText(Vector3D pos, int damage);
    void SpawnHitParticles(Vector3D pos);
    void DoHitstop(float duration);
    void DoScreenshake(float duration);
};

}
