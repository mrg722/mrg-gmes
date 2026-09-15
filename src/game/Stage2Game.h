#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include <vector>

namespace district_fury {
class Stage2Game {
public:
    Stage2Game();
    void Init();
    void Update(float dt);
    void Draw() const;
private:
    struct Projectile { Vector3D pos; float vx; float life; int damage; bool active; };
    struct Particle { Vector3D pos; Vector2 vel; float life; float maxLife; float size; Color color; };
    enum class Flow { Intro, Combat, BossIntro, Boss, Clear, GameOver, Pause };
    enum class BossAttack { None, Saw, Slam, Ram, Overdrive };
    struct Grinder { Vector3D pos{0,575,0}; int hp{1100}; int maxHp{1100}; int phase{1}; float attackTimer{1}; float elapsed{0}; float invuln{0}; bool defeated{false}; BossAttack attack{BossAttack::None}; };
    Player player;
    std::vector<StreetEnemy> enemies;
    std::vector<Projectile> projectiles;
    std::vector<Particle> particles;
    Flow flow{Flow::Intro};
    Grinder boss;
    float cameraX{640}; float stageTime{0}; float hitstop{0}; float shake{0}; float comboTimer{0};
    int combo{0}; int maxCombo{0}; int damageTaken{0}; int defeated{0}; int wave{0};
    bool arenaLocked{false}; float bannerTimer{0};
    void BuildStage(); void SpawnWave(int id); void UpdateCombat(float dt); void UpdateBoss(float dt); void UpdateProjectiles(float dt); void UpdateParticles(float dt);
    void HandlePlayerHits(); void HandleEnemyHits(); void HandleBossHits(); void SpawnEnergy(); void SpawnImpact(Vector3D pos, Color color, bool heavy); void EnterBoss(); void DefeatBoss();
    void DrawWorld() const; void DrawHUD() const; void DrawBoss() const; void DrawClear() const; void DrawArenaLock() const;
};
}
