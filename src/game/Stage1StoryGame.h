#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include <string>
#include <vector>

namespace district_fury {

enum class StoryFlow { Menu, Controls, Intro, Combat, SubBossIntro, ScenarioClear, BossIntro, Boss, StageClear, GameOver, Pause };
enum class StoryBossAttack { None, ChainSwing, GroundSmash, Charge, PowerWave, Frenzy };

struct StoryProjectile {
    Vector3D position;
    float velocity;
    float life;
    float radius;
    int damage;
    bool active;
    bool fromBoss;
};

struct StoryParticle {
    Vector3D position;
    Vector2 velocity;
    float life;
    float maxLife;
    float size;
    Color color;
};

struct StoryBoss {
    Vector3D position{0, 575, 0};
    int hp{1200};
    int maxHp{1200};
    int phase{1};
    float stateTimer{0};
    float attackTimer{1.2f};
    float attackElapsed{0};
    float invulnerability{0};
    float blockTimer{0};
    float powerTimer{2.0f};
    bool blocking{false};
    bool defeated{false};
    StoryBossAttack attack{StoryBossAttack::None};
};

class Stage1StoryGame {
public:
    Stage1StoryGame();
    void Init();
    void Update(float dt);
    void Draw() const;

private:
    Player player;
    std::vector<StreetEnemy> enemies;
    std::vector<StoryProjectile> projectiles;
    std::vector<StoryParticle> particles;
    StoryFlow flow{StoryFlow::Menu};
    StoryBoss boss;

    int scenario{1};
    int wave{0};
    int combo{0};
    int maxCombo{0};
    int defeated{0};
    int damageTaken{0};
    int score{0};
    int xp{0};
    int coins{0};
    int gems{0};
    int level{1};
    int bestScore{0};
    int bestRank{0};
    float cameraX{640};
    float stageTime{0};
    float comboTimer{0};
    float hitstop{0};
    float shake{0};
    float bannerTimer{0};
    float transitionTimer{0};
    float storyTimer{0};
    bool arenaLocked{false};
    bool scenarioBossSpawned{false};
    bool finalBossSpawned{false};
    bool stageComplete{false};
    bool saveLoaded{false};
    Difficulty difficulty{Difficulty::Normal};
    std::string savePath{"district_fury_save.dat"};
    std::string storyMessage;

    void ResetRun();
    void BuildScenario(int id);
    void SpawnWave(int id);
    void SpawnScenarioBoss();
    void EnterFinalBoss();
    void DefeatFinalBoss();
    void ApplyDifficulty();
    void UpdateCombat(float dt);
    void UpdateBoss(float dt);
    void UpdateProjectiles(float dt);
    void UpdateParticles(float dt);
    void HandlePlayerHits();
    void HandleEnemyHits();
    void HandleBossHits();
    void SpawnEnergyProjectile();
    void SpawnBossPower();
    void SpawnImpact(Vector3D pos, Color color, bool heavy);
    void AdvanceScenario();
    bool ScenarioWaveCleared() const;
    bool AllCurrentEnemiesDefeated() const;
    int ScenarioStartX() const;
    int ScenarioEndX() const;
    const char* ScenarioName() const;
    const char* ScenarioObjective() const;
    int CalculateRank() const;
    int CalculateScore() const;
    const char* RankText() const;
    const char* DifficultyText() const;
    void LoadSave();
    void SaveProgress();
    void DrawWorld() const;
    void DrawHUD() const;
    void DrawMenu() const;
    void DrawControls() const;
    void DrawPause() const;
    void DrawGameOver() const;
    void DrawScenarioClear() const;
    void DrawBoss() const;
    void DrawStageClear() const;
    void DrawArenaLock() const;
    void DrawScenarioArt() const;
};

}
