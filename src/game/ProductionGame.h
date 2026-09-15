#pragma once
#include "game/Player.h"
#include "game/Scene.h"
#include "game/StreetEnemy.h"
#include <string>
#include <vector>

namespace district_fury {

enum class ProductionFlow { Menu, Intro, Combat, BossIntro, Boss, StageClear, GameOver, Pause };
enum class Difficulty { Easy, Normal, Hard };
enum class BossAttack { None, ChainSwing, GroundSmash, Charge, Frenzy };

struct EnergyProjectile { Vector3D position; float velocity; float life; float radius; int damage; bool active; };
struct BossState { Vector3D position; Vector3D velocity; int hp; int maxHp; int phase; float stateTimer; float attackTimer; float attackElapsed; float invulnerability; bool hit; bool defeated; BossAttack attack; };
struct ProductionParticle { Vector3D position; Vector2 velocity; float life; float maxLife; float size; Color color; };

class ProductionGame {
public:
    ProductionGame();
    ~ProductionGame();
    void Init();
    void Update(float dt);
    void Draw() const;

private:
    Player player;
    Scene scene;
    std::vector<StreetEnemy> enemies;
    std::vector<EnergyProjectile> projectiles;
    std::vector<ProductionParticle> particles;
    ProductionFlow flow;
    Difficulty difficulty;
    BossState boss;
    float cameraX, introTimer, encounterBannerTimer, bossBannerTimer, stageTime, hitstop, shake, score, comboTimer;
    int combo, maxCombo, defeated, totalEnemies, damageTaken, stageReward, xp, coins, gems, level;
    bool stageComplete, bossSpawned, saveLoaded, arenaLocked;
    int currentWave, waveDefeated;
    std::string savePath;

    void ResetRun();
    void BuildStage();
    void SpawnWave(int wave);
    void ActivateNearbyEnemies();
    bool ActiveWaveCleared() const;
    bool AllStreetEnemiesCleared() const;
    void UpdateCombat(float dt);
    void UpdateProjectiles(float dt);
    void UpdateBoss(float dt);
    void UpdateParticles(float dt);
    void HandlePlayerHits();
    void HandleEnemyHits();
    void HandleBossHits();
    void SpawnEnergyProjectile();
    void SpawnImpact(Vector3D pos, Color color, bool heavy);
    void ApplyDifficulty();
    void EnterBoss();
    void DefeatBoss();
    void FinishStage();
    void LoadSave();
    void SaveProgress() const;
    void ResetSave();
    int CalculateRank() const;
    int CalculateScore() const;
    const char* RankText() const;
    const char* DifficultyText() const;
    void DrawWorld() const;
    void DrawHUD() const;
    void DrawMenu() const;
    void DrawPause() const;
    void DrawGameOver() const;
    void DrawStageClear() const;
    void DrawBoss() const;
    void DrawArenaLock() const;
};

}
