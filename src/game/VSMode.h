#pragma once
#include "game/Player.h"
#include "game/StreetEnemy.h"
#include <array>
#include <vector>

namespace district_fury {

enum class VSFlow { Select, Fight };

class VSMode {
public:
    VSMode();
    void Init();
    void Update(float dt);
    void Draw() const;
    bool ShouldExit() const;
    void ClearExit();

private:
    VSFlow flow{VSFlow::Select};
    int stage{0};
    int scenario{0};
    int enemyCount{1};
    int cursor{0};
    bool exitRequested{false};
    bool playerDefeated{false};
    Player player;
    std::array<StreetEnemyType, 4> enemyTypes{
        StreetEnemyType::Punk,
        StreetEnemyType::Brute,
        StreetEnemyType::Charger,
        StreetEnemyType::Enforcer
    };
    std::vector<StreetEnemy> enemies;
    float shake{0.0f};

    void ResetFight();
    void StartFight();
    void UpdateFight(float dt);
    void DrawSelection() const;
    void DrawFight() const;
    void DrawBackground() const;
    void DrawHud() const;
    int ScenarioCount() const;
    const char* StageName() const;
    const char* ScenarioText() const;
    const char* BackgroundKey() const;
    static const char* EnemyTypeName(StreetEnemyType type);
};

}
