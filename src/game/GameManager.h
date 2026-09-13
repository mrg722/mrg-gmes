#pragma once
#include "game/Player.h"
#include "game/Scene.h"
#include "game/Enemy.h"
#include <vector>

namespace district_fury {

enum class GameFlowState {
    Start,
    Combat,
    Pause,
    Win,
    GameOver
};

struct DamageText {
    Vector3D position;
    int amount;
    float timer;
    float maxTime;
    bool critical;
};

struct Particle {
    Vector3D position;
    Vector2 velocity;
    float timer;
    float maxTime;
    Color color;
    float size;
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
    float introTimer;
    float comboTimer;
    float elapsedTime;
    int maxCombo;

    GameManager();
    void Init();
    void Update(float dt);
    void Draw() const;

    void SpawnDamageText(Vector3D pos, int damage, bool critical = false);
    void SpawnHitParticles(Vector3D pos, AttackType type, bool heavy);
    void DoHitstop(float duration);
    void DoScreenshake(float duration);
};

}
