#pragma once
#include "game/Player.h"
#include "game/Scene.h"
#include "game/StreetEnemy.h"
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
    std::vector<StreetEnemy> enemies;
    Scene scene;
    GameFlowState flowState;

    std::vector<DamageText> damageTexts;
    std::vector<Particle> particles;

    float hitstopTimer;
    float screenshakeTimer;
    float introTimer;
    float elapsedTime;
    float cameraX;
    int maxCombo;
    int defeatedEnemies;

    GameManager();
    void Init();
    void Update(float dt);
    void Draw() const;

    void SpawnDamageText(Vector3D pos, int damage, bool critical = false);
    void SpawnHitParticles(Vector3D pos, AttackType type, bool heavy);
    void DoHitstop(float duration);
    void DoScreenshake(float duration);
    bool AllEnemiesDefeated() const;
    int RemainingEnemies() const;
};

}
