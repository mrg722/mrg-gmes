#include "game/Stage3Game.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {
constexpr float kPi = 3.14159265359f;
constexpr float kLaneMin = 505.0f;
constexpr float kLaneMax = 625.0f;

struct Spawn { float x; float y; StreetEnemyType type; };
const Spawn kScenarios[3][5] = {
    {{620,570,StreetEnemyType::UrbanNinja},{820,535,StreetEnemyType::Enforcer},{1020,610,StreetEnemyType::Punk},{1190,555,StreetEnemyType::UrbanNinja},{0,0,StreetEnemyType::Punk}},
    {{2050,575,StreetEnemyType::ChemicalSoldier},{2220,525,StreetEnemyType::Mutant},{2390,610,StreetEnemyType::ArmoredGuard},{2560,550,StreetEnemyType::ChemicalSoldier},{0,0,StreetEnemyType::Punk}},
    {{3490,570,StreetEnemyType::ArmoredGuard},{3660,525,StreetEnemyType::UrbanNinja},{3830,610,StreetEnemyType::Enforcer},{4000,550,StreetEnemyType::Mutant},{4170,590,StreetEnemyType::ArmoredGuard}}
};

Color Alpha(Color c, float a) {
    c.a = static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
    return c;
}
}

Stage3Game::Stage3Game() = default;

void Stage3Game::Init() {
    ResetRun();
    flow = Flow::Intro;
}

void Stage3Game::ResetRun() {
    player.Reset();
    player.position = {180,585,0};
    enemies.clear();
    projectiles.clear();
    particles.clear();
    scenario = 1;
    combo = 0;
    maxCombo = 0;
    defeated = 0;
    damageTaken = 0;
    score = 0;
    stageTime = 0;
    comboTimer = 0;
    hitstop = 0;
    shake = 0;
    bannerTimer = 0;
    transitionTimer = 0;
    cameraX = 640;
    scenarioGatekeeperSpawned = false;
    bossSpawned = false;
    stageComplete = false;
    boss = TitanX{};
    storyMessage = "Rayden enters Astra Tower. The chain network ends in the corporate command floor.";
    BuildScenario(1);
}

int Stage3Game::ScenarioStartX() const {
    return scenario == 1 ? 180 : scenario == 2 ? 1850 : 3290;
}

int Stage3Game::ScenarioEndX() const {
    return scenario == 1 ? 1700 : scenario == 2 ? 3140 : 5000;
}

const char* Stage3Game::ScenarioName() const {
    switch (scenario) {
        case 1: return "ASTRA TOWER // PUBLIC ATRIUM";
        case 2: return "ASTRA TOWER // RESEARCH FLOOR";
        default: return "ASTRA TOWER // EXECUTIVE CORE";
    }
}

const char* Stage3Game::ScenarioObjective() const {
    switch (scenario) {
        case 1: return "Break the security cordon.";
        case 2: return "Cross the restricted research floor.";
        default: return "Reach the executive core and expose Titan-X.";
    }
}

const char* Stage3Game::DifficultyText() const {
    return difficulty == Difficulty::Easy ? "EASY" : difficulty == Difficulty::Hard ? "HARD" : "NORMAL";
}

void Stage3Game::ApplyDifficulty() {
    const float hpMult = difficulty == Difficulty::Easy ? 0.84f : difficulty == Difficulty::Hard ? 1.22f : 1.0f;
    const float dmgMult = difficulty == Difficulty::Easy ? 0.82f : difficulty == Difficulty::Hard ? 1.20f : 1.0f;
    for (auto& enemy : enemies) {
        enemy.hp = std::max(1, static_cast<int>(std::round(enemy.hp * hpMult)));
        enemy.maxHp = enemy.hp;
        enemy.attackDamage = std::max(1.0f, enemy.attackDamage * dmgMult);
    }
    boss.maxHp = difficulty == Difficulty::Easy ? 1250 : difficulty == Difficulty::Hard ? 1800 : 1500;
    boss.hp = boss.maxHp;
}

void Stage3Game::BuildScenario(int id) {
    enemies.clear();
    scenarioGatekeeperSpawned = false;
    const int index = std::clamp(id - 1, 0, 2);
    for (const auto& spawn : kScenarios[index]) {
        if (spawn.x <= 0) continue;
        StreetEnemy enemy;
        enemy.Init({spawn.x, spawn.y, 0}, spawn.type);
        enemy.active = false;
        enemies.push_back(enemy);
    }
    ApplyDifficulty();
}

bool Stage3Game::AllEnemiesDefeated() const {
    return !enemies.empty() && std::all_of(enemies.begin(), enemies.end(), [](const StreetEnemy& e) { return e.IsDefeated(); });
}

void Stage3Game::SpawnScenarioGatekeeper() {
    if (scenario >= 3) {
        AdvanceScenario();
        return;
    }
    scenarioGatekeeperSpawned = true;
    flow = Flow::Gatekeeper;
    bannerTimer = 1.7f;
    arenaLocked = false;
    enemies.clear();
    StreetEnemy gatekeeper;
    const StreetEnemyType type = scenario == 1 ? StreetEnemyType::ArmoredGuard : StreetEnemyType::Mutant;
    gatekeeper.Init({static_cast<float>(ScenarioEndX() - 180), 570, 0}, type);
    gatekeeper.active = true;
    gatekeeper.hp += scenario == 1 ? 320 : 450;
    gatekeeper.maxHp = gatekeeper.hp;
    gatekeeper.attackDamage += scenario == 1 ? 5.0f : 8.0f;
    enemies.push_back(gatekeeper);
    storyMessage = scenario == 1
        ? "ASTRA SECURITY CAPTAIN: unauthorized access detected."
        : "RESEARCH WARDEN: the tower will not surrender its prototype floor.";
}

void Stage3Game::AdvanceScenario() {
    if (scenario >= 3) {
        EnterBoss();
        return;
    }
    ++scenario;
    combo = 0;
    comboTimer = 0;
    transitionTimer = 2.2f;
    bannerTimer = 2.2f;
    flow = Flow::Intro;
    BuildScenario(scenario);
    player.position.x = static_cast<float>(ScenarioStartX());
    if (scenario == 2) storyMessage = "The public atrium was only a filter. Research access is deeper inside the tower.";
    else storyMessage = "The prototype floor is breached. One door remains: the executive core.";
}

void Stage3Game::EnterBoss() {
    bossSpawned = true;
    boss = TitanX{};
    boss.pos = {5350,575,0};
    ApplyDifficulty();
    boss.phase = 1;
    boss.attackTimer = 1.3f;
    boss.powerTimer = 2.0f;
    boss.shieldTimer = 0.7f;
    boss.blocking = false;
    player.position.x = 4880;
    flow = Flow::BossIntro;
    bannerTimer = 2.8f;
    storyMessage = "TITAN-X // ASTRA CORE: military-grade combat AI activated.";
}

void Stage3Game::DefeatBoss() {
    boss.hp = 0;
    boss.defeated = true;
    boss.blocking = false;
    arenaLocked = false;
    stageComplete = true;
    flow = Flow::Clear;
    score += 5000 + maxCombo * 50;
    defeated += 1;
    SpawnImpact(boss.pos, {80,220,255,255}, true);
}

void Stage3Game::SpawnImpact(Vector3D pos, Color color, bool heavy) {
    const int count = heavy ? 30 : 14;
    for (int i = 0; i < count; ++i) {
        const float a = static_cast<float>(i) / count * 2.0f * kPi;
        const float speed = static_cast<float>(GetRandomValue(70, heavy ? 360 : 220));
        particles.push_back({pos, {std::cos(a) * speed, std::sin(a) * speed}, heavy ? .5f : .28f, heavy ? .5f : .28f, static_cast<float>(GetRandomValue(3, heavy ? 9 : 6)), color});
    }
}

void Stage3Game::SpawnEnergyProjectile() {
    const float direction = player.facing == Facing::Right ? 1.0f : -1.0f;
    projectiles.push_back({{player.position.x + direction * 70, player.position.y - 70, 0}, direction * 760.0f, .9f, 22.0f, player.isRageMode ? 34 : 24, false, true});
    SpawnImpact({player.position.x + direction * 45, player.position.y - 70, 0}, {55,215,255,255}, true);
}

void Stage3Game::SpawnBossProjectile(int damage, float speed) {
    const float direction = player.position.x >= boss.pos.x ? -1.0f : 1.0f;
    projectiles.push_back({{boss.pos.x + direction * 90, boss.pos.y - 70, 0}, direction * speed, 1.45f, 25.0f, damage, true, true});
    SpawnImpact({boss.pos.x + direction * 55, boss.pos.y - 70, 0}, {255,75,190,255}, true);
}

void Stage3Game::UpdateProjectiles(float dt) {
    for (auto& projectile : projectiles) {
        if (!projectile.active) continue;
        projectile.pos.x += projectile.vx * dt;
        projectile.life -= dt;
        if (projectile.life <= 0) { projectile.active = false; continue; }
        const CombatBox box{projectile.pos.x - projectile.radius, projectile.pos.y - projectile.radius, projectile.radius * 2, projectile.radius * 2};
        if (projectile.fromBoss) {
            if (box.Intersects(player.GetHurtbox()) && player.state != PlayerState::Defeat && player.dashInvulnerability <= 0) {
                const int before = player.hp;
                player.TakeDamage(projectile.damage);
                damageTaken += before - player.hp;
                projectile.active = false;
                combo = 0;
                comboTimer = 0;
                SpawnImpact(player.position, {255,65,150,255}, true);
                hitstop = .10f;
                shake = .18f;
            }
        } else {
            for (auto& enemy : enemies) {
                if (enemy.active && !enemy.IsDefeated() && box.Intersects(enemy.GetHurtbox())) {
                    enemy.TakeDamage(projectile.damage, {projectile.vx > 0 ? 420.0f : -420.0f, 0, 0});
                    projectile.active = false;
                    ++combo;
                    comboTimer = 1.0f;
                    maxCombo = std::max(maxCombo, combo);
                    score += 180 + combo * 10;
                    SpawnImpact(projectile.pos, {55,215,255,255}, true);
                    hitstop = .10f;
                    shake = .14f;
                    break;
                }
            }
            if (flow == Flow::Boss && !boss.defeated && projectile.active) {
                const CombatBox target{boss.pos.x - 80, boss.pos.y - 150, 160, 150};
                if (box.Intersects(target) && boss.invuln <= 0) {
                    int damage = projectile.damage;
                    if (boss.blocking) damage = std::max(1, damage / 6);
                    boss.hp = std::max(0, boss.hp - damage);
                    boss.invuln = .12f;
                    projectile.active = false;
                    ++combo;
                    comboTimer = 1.0f;
                    maxCombo = std::max(maxCombo, combo);
                    score += boss.blocking ? 100 : 300;
                    SpawnImpact(projectile.pos, boss.blocking ? Color{110,180,230,255} : Color{255,185,55,255}, true);
                    hitstop = .12f;
                    shake = .16f;
                    if (boss.hp <= 0) DefeatBoss();
                }
            }
        }
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const Projectile& p) { return !p.active; }), projectiles.end());
}

void Stage3Game::HandlePlayerHits() {
    if (!player.AttackIsActive() || player.hasHit || player.attackType == AttackType::Energy) return;
    const CombatBox hit = player.GetAttackHitbox();
    for (auto& enemy : enemies) {
        if (!enemy.active || enemy.IsDefeated() || !hit.Intersects(enemy.GetHurtbox())) continue;
        const bool heavy = player.attackType == AttackType::Kick || player.comboStep >= 2;
        const int damage = player.GetAttackDamage() + (player.isRageMode ? 6 : 0);
        const float direction = player.facing == Facing::Right ? 1.0f : -1.0f;
        enemy.TakeDamage(damage, {direction * player.GetAttackKnockback(), 0, 0});
        player.hasHit = true;
        ++combo;
        comboTimer = 1.0f;
        maxCombo = std::max(maxCombo, combo);
        score += damage * 10 + combo * 8;
        SpawnImpact({enemy.position.x, enemy.position.y - 68, 0}, heavy ? Color{255,150,45,255} : Color{255,235,150,255}, heavy);
        hitstop = heavy ? .11f : .065f;
        shake = heavy ? .14f : .07f;
        break;
    }
}

void Stage3Game::HandleEnemyHits() {
    if (player.state == PlayerState::Defeat) return;
    int attackers = 0;
    for (auto& enemy : enemies) {
        if (!enemy.active || enemy.IsDefeated() || !enemy.AttackIsActive()) continue;
        if (++attackers > 2) continue;
        if (!enemy.hasHit && enemy.GetAttackHitbox().Intersects(player.GetHurtbox()) && player.dashInvulnerability <= 0) {
            const int before = player.hp;
            player.TakeDamage(static_cast<int>(enemy.attackDamage));
            damageTaken += before - player.hp;
            enemy.hasHit = true;
            combo = 0;
            comboTimer = 0;
            SpawnImpact(player.position, {255,65,120,255}, false);
            hitstop = .085f;
            shake = .12f;
        }
    }
}

void Stage3Game::HandleBossHits() {
    if (flow != Flow::Boss || boss.defeated || boss.invuln > 0 || !player.AttackIsActive() || player.hasHit || player.attackType == AttackType::Energy) return;
    const CombatBox hit = player.GetAttackHitbox();
    const CombatBox target{boss.pos.x - 82, boss.pos.y - 150, 164, 150};
    if (!hit.Intersects(target)) return;
    int damage = player.GetAttackDamage() + (player.isRageMode ? 9 : 0);
    if (boss.blocking) {
        damage = std::max(1, damage / 6);
        player.velocity.x += player.facing == Facing::Right ? -150.0f : 150.0f;
        SpawnImpact({boss.pos.x, boss.pos.y - 90, 0}, {105,180,230,255}, true);
    } else {
        SpawnImpact({boss.pos.x, boss.pos.y - 90, 0}, {255,175,55,255}, true);
    }
    boss.hp = std::max(0, boss.hp - damage);
    boss.invuln = .11f;
    player.hasHit = true;
    ++combo;
    comboTimer = 1.0f;
    maxCombo = std::max(maxCombo, combo);
    score += boss.blocking ? 120 : 280 + combo * 12;
    hitstop = .11f;
    shake = .18f;
    if (boss.hp <= 0) DefeatBoss();
}

void Stage3Game::UpdateCombat(float dt) {
    if (comboTimer > 0) comboTimer -= dt; else combo = 0;
    player.Update(dt);
    static PlayerState previousState = PlayerState::Idle;
    static AttackType previousAttack = AttackType::None;
    if (player.state == PlayerState::Attack && player.attackType == AttackType::Energy && (previousState != PlayerState::Attack || previousAttack != AttackType::Energy)) SpawnEnergyProjectile();
    previousState = player.state;
    previousAttack = player.attackType;

    for (auto& enemy : enemies) {
        if (enemy.active) enemy.Update(dt, player);
    }
    HandlePlayerHits();
    UpdateProjectiles(dt);
    HandleEnemyHits();
    HandleBossHits();

    const float left = static_cast<float>(ScenarioStartX());
    const float right = static_cast<float>(ScenarioEndX() - 150);
    player.position.x = std::clamp(player.position.x, left, right);
    player.position.y = std::clamp(player.position.y, kLaneMin, kLaneMax);

    if (AllEnemiesDefeated() && !scenarioGatekeeperSpawned) {
        ++defeated;
        SpawnScenarioGatekeeper();
        return;
    }
    if (scenarioGatekeeperSpawned && AllEnemiesDefeated()) {
        defeated += 1;
        AdvanceScenario();
        return;
    }
    if (player.state == PlayerState::Defeat) flow = Flow::GameOver;
}

void Stage3Game::UpdateBoss(float dt) {
    if (boss.invuln > 0) boss.invuln -= dt;
    if (boss.shieldTimer > 0) boss.shieldTimer -= dt;
    if (boss.attackTimer > 0) boss.attackTimer -= dt;
    if (boss.powerTimer > 0) boss.powerTimer -= dt;
    if (boss.dashTimer > 0) boss.dashTimer -= dt;

    if (flow == Flow::BossIntro) {
        bannerTimer -= dt;
        player.position.x = std::min(player.position.x, boss.pos.x - 270.0f);
        if (bannerTimer <= 0) flow = Flow::Boss;
        return;
    }
    if (boss.defeated) return;

    const float ratio = static_cast<float>(boss.hp) / boss.maxHp;
    const int nextPhase = ratio <= .33f ? 3 : ratio <= .66f ? 2 : 1;
    if (nextPhase != boss.phase) {
        boss.phase = nextPhase;
        boss.shieldTimer = 1.0f;
        boss.blocking = true;
        SpawnImpact(boss.pos, {255,70,190,255}, true);
        shake = .25f;
    }

    const bool playerThreat = player.AttackIsActive() || player.attackType == AttackType::Energy;
    if (boss.shieldTimer <= 0) {
        const float guardChance = boss.phase == 1 ? .34f : boss.phase == 2 ? .46f : .58f;
        if (playerThreat && boss.attackTimer < .25f && GetRandomValue(0,1000) < static_cast<int>(guardChance * 1000.0f)) {
            boss.blocking = true;
            boss.shieldTimer = boss.phase == 3 ? .75f : .55f;
        } else if (boss.blocking && boss.attackTimer <= 0) {
            boss.blocking = false;
        }
    }

    if (boss.attackTimer <= 0 && !boss.blocking) {
        boss.attackTimer = boss.phase == 3 ? .72f : boss.phase == 2 ? .95f : 1.18f;
        const int attack = GetRandomValue(0, boss.phase == 3 ? 3 : 2);
        if (attack == 0) {
            boss.dashTimer = boss.phase == 3 ? .34f : .26f;
            const float direction = player.position.x > boss.pos.x ? 1.0f : -1.0f;
            boss.pos.x += direction * (boss.phase == 3 ? 220.0f : 170.0f);
            SpawnImpact(boss.pos, {255,80,190,255}, false);
        } else if (attack == 1) {
            SpawnBossProjectile(boss.phase == 3 ? 34 : boss.phase == 2 ? 29 : 24, boss.phase == 3 ? 850.0f : 680.0f);
        } else {
            const float distance = std::abs(player.position.x - boss.pos.x);
            if (distance < 270.0f) {
                const int before = player.hp;
                player.TakeDamage(boss.phase == 3 ? 42 : boss.phase == 2 ? 34 : 28);
                damageTaken += before - player.hp;
                combo = 0;
                comboTimer = 0;
                SpawnImpact(player.position, {255,65,150,255}, true);
                hitstop = .11f;
                shake = .2f;
            } else {
                SpawnBossProjectile(boss.phase == 3 ? 30 : 24, 760.0f);
            }
        }
    }

    boss.pos.x = std::clamp(boss.pos.x, 5050.0f, 5680.0f);
    boss.pos.y = std::clamp(boss.pos.y, kLaneMin, kLaneMax);
    HandleBossHits();
    if (player.state == PlayerState::Defeat) flow = Flow::GameOver;
}

void Stage3Game::UpdateParticles(float dt) {
    for (auto& particle : particles) {
        particle.pos.x += particle.vel.x * dt;
        particle.pos.y += particle.vel.y * dt;
        particle.vel.y += 180.0f * dt;
        particle.life -= dt;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const Particle& p) { return p.life <= 0; }), particles.end());
}

void Stage3Game::Update(float dt) {
    if (IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE)) {
        if (flow != Flow::Clear && flow != Flow::GameOver && flow != Flow::Intro && flow != Flow::BossIntro) {
            flow = flow == Flow::Pause ? Flow::Combat : Flow::Pause;
        }
    }
    if (flow == Flow::Pause) {
        if (IsKeyPressed(KEY_ENTER)) flow = Flow::Combat;
        return;
    }
    if (flow == Flow::Intro) {
        if (IsKeyPressed(KEY_ONE)) difficulty = Difficulty::Easy;
        if (IsKeyPressed(KEY_TWO)) difficulty = Difficulty::Normal;
        if (IsKeyPressed(KEY_THREE)) difficulty = Difficulty::Hard;
        if (IsKeyPressed(KEY_ENTER)) {
            ResetRun();
            flow = Flow::Combat;
            bannerTimer = 1.8f;
        }
        if (transitionTimer > 0) transitionTimer -= dt;
    } else if (flow == Flow::Combat || flow == Flow::Gatekeeper) {
        stageTime += dt;
        if (bannerTimer > 0) bannerTimer -= dt;
        if (hitstop > 0) hitstop -= dt;
        else UpdateCombat(dt);
    } else if (flow == Flow::BossIntro || flow == Flow::Boss) {
        stageTime += dt;
        if (hitstop > 0) hitstop -= dt;
        else {
            player.Update(dt);
            UpdateBoss(dt);
            UpdateProjectiles(dt);
            if (player.state == PlayerState::Defeat) flow = Flow::GameOver;
        }
    } else if (flow == Flow::Clear || flow == Flow::GameOver) {
        if (IsKeyPressed(KEY_ENTER)) {
            ResetRun();
            flow = Flow::Combat;
        }
    }

    if (comboTimer > 0) comboTimer -= dt; else if (combo > 0) combo = 0;
    UpdateParticles(dt);
    cameraX = std::clamp(player.position.x, 640.0f, 5360.0f);
}

void Stage3Game::DrawWorld() const {
    const Camera2D camera{{cameraX, 360}, {640,360}, 0, 1.0f};
    BeginMode2D(camera);

    DrawRectangle(-200, 0, 6400, 720, {6,8,18,255});
    DrawRectangle(-200, 160, 6400, 300, {11,18,38,255});
    for (int x = 0; x < 6200; x += 320) {
        DrawRectangle(x, 170, 270, 180, {15,25,48,255});
        DrawRectangle(x + 18, 195, 96, 72, {25,70,105,255});
        DrawRectangle(x + 138, 195, 96, 72, {105,42,104,255});
        DrawLine(x, 350, x + 270, 350, {70,95,130,180});
    }
    for (int x = 0; x < 6200; x += 160) {
        DrawLineEx({static_cast<float>(x), 365}, {static_cast<float>(x + 80), 430}, 4, {35,55,85,220});
        DrawLineEx({static_cast<float>(x + 80), 430}, {static_cast<float>(x + 160), 365}, 4, {35,55,85,220});
    }
    DrawRectangle(-200, 430, 6400, 220, {9,12,22,255});
    DrawRectangle(-200, 645, 6400, 75, {3,5,10,255});
    for (int x = 0; x < 6200; x += 240) {
        DrawRectangle(x, 455, 12, 170, {45,55,80,255});
        DrawRectangle(x + 55, 470, 170, 9, {100,40,115,220});
        DrawCircle(x + 30, 445, 5, {70,220,255,220});
    }

    for (const auto& particle : particles) {
        const float alpha = particle.life / particle.maxLife;
        DrawCircleV({particle.pos.x, particle.pos.y - particle.pos.z}, particle.size, Alpha(particle.color, alpha));
    }
    for (const auto& projectile : projectiles) {
        const Color c = projectile.fromBoss ? Color{255,65,190,255} : Color{60,220,255,255};
        DrawCircleV({projectile.pos.x, projectile.pos.y - 70}, projectile.radius, Alpha(c, .9f));
        DrawCircleV({projectile.pos.x, projectile.pos.y - 70}, projectile.radius * .42f, {245,245,255,255});
    }
    for (const auto& enemy : enemies) if (enemy.active) enemy.Draw();
    if (!bossSpawned || !boss.defeated) player.Draw();
    if (flow == Flow::BossIntro || flow == Flow::Boss || flow == Flow::Clear) DrawBoss();
    EndMode2D();
}

void Stage3Game::DrawBoss() const {
    if (!bossSpawned || boss.defeated) return;
    const float x = boss.pos.x;
    const float y = boss.pos.y - 75;
    const Color core = boss.phase == 3 ? Color{255,55,175,255} : boss.phase == 2 ? Color{125,105,255,255} : Color{65,215,255,255};
    if (boss.blocking) DrawCircleV({x, y,}, 105, Alpha({90,170,255,255}, .22f));
    DrawRectangle(static_cast<int>(x - 62), static_cast<int>(y - 75), 124, 150, {32,40,62,255});
    DrawRectangle(static_cast<int>(x - 48), static_cast<int>(y - 95), 96, 28, {55,65,92,255});
    DrawRectangle(static_cast<int>(x - 38), static_cast<int>(y - 67), 76, 45, {14,18,30,255});
    DrawCircle(static_cast<int>(x), static_cast<int>(y - 45), 16, core);
    DrawRectangle(static_cast<int>(x - 88), static_cast<int>(y - 55), 24, 95, {48,58,82,255});
    DrawRectangle(static_cast<int>(x + 64), static_cast<int>(y - 55), 24, 95, {48,58,82,255});
    DrawRectangle(static_cast<int>(x - 48), static_cast<int>(y + 72), 34, 52, {42,48,68,255});
    DrawRectangle(static_cast<int>(x + 14), static_cast<int>(y + 72), 34, 52, {42,48,68,255});
    DrawLineEx({x - 80, y + 115}, {x - 48, y + 150}, 10, core);
    DrawLineEx({x + 80, y + 115}, {x + 48, y + 150}, 10, core);
}

void Stage3Game::DrawHUD() const {
    DrawRectangle(18, 16, 420, 88, {5,8,16,225});
    DrawText("DISTRICT FURY // ASTRA TOWER", 32, 27, 18, {180,220,235,255});
    DrawText(ScenarioName(), 32, 51, 15, {255,115,205,255});
    DrawText(ScenarioObjective(), 32, 73, 13, {195,205,220,255});

    DrawRectangle(32, 120, 250, 18, {25,28,38,255});
    DrawRectangle(32, 120, static_cast<int>(250.0f * std::max(0.0f, static_cast<float>(player.hp) / player.maxHp)), 18, {70,210,120,255});
    DrawText(TextFormat("HP %d / %d", player.hp, player.maxHp), 38, 121, 12, RAYWHITE);
    DrawRectangle(32, 144, 250, 12, {25,28,38,255});
    DrawRectangle(32, 144, static_cast<int>(250.0f * std::max(0.0f, static_cast<float>(player.rage) / player.maxRage)), 12, {255,85,180,255});
    DrawText(TextFormat("RAGE %d%%", player.rage), 38, 143, 10, RAYWHITE);

    if (combo > 1) {
        DrawText(TextFormat("%d HIT", combo), 1050, 112, 30, {255,205,75,255});
        DrawText(TextFormat("MAX %d", maxCombo), 1060, 143, 13, {210,220,235,255});
    }
    DrawText(TextFormat("SCORE %06d", score), 1040, 24, 16, {240,240,245,255});
    DrawText(TextFormat("TIME %05.1f", stageTime), 1040, 46, 13, {170,190,210,255});
    DrawText(TextFormat("DIFF %s", DifficultyText()), 1040, 66, 12, {170,190,210,255});

    if (flow == Flow::BossIntro || flow == Flow::Boss || flow == Flow::Clear) {
        DrawRectangle(380, 18, 520, 54, {7,8,18,230});
        DrawText("TITAN-X // ASTRA CORE", 515, 25, 20, {255,90,190,255});
        DrawRectangle(430, 51, 420, 12, {28,30,42,255});
        DrawRectangle(430, 51, static_cast<int>(420.0f * std::max(0.0f, static_cast<float>(boss.hp) / boss.maxHp)), 12, {95,190,255,255});
        DrawText(TextFormat("PHASE %d", boss.phase), 875, 49, 12, {190,205,220,255});
    }
}

void Stage3Game::DrawClear() const {
    DrawRectangle(0, 0, 1280, 720, {4,7,14,235});
    DrawText("ASTRA TOWER // CLEARED", 355, 135, 34, {80,225,255,255});
    DrawText("TITAN-X has been shut down.", 475, 190, 18, {225,230,240,255});
    const float rankScore = score + maxCombo * 120.0f - damageTaken * 3.0f - stageTime * 2.0f;
    const char* rank = rankScore > 12000 ? "SSS" : rankScore > 9000 ? "SS" : rankScore > 7000 ? "S" : rankScore > 5000 ? "A" : rankScore > 3500 ? "B" : rankScore > 2000 ? "C" : "D";
    DrawText(TextFormat("RANK %s", rank), 550, 255, 42, {255,205,75,255});
    DrawText(TextFormat("SCORE  %06d", score), 525, 330, 18, RAYWHITE);
    DrawText(TextFormat("MAX COMBO  %d", maxCombo), 520, 360, 16, {205,215,230,255});
    DrawText(TextFormat("DAMAGE TAKEN  %d", damageTaken), 515, 390, 16, {205,215,230,255});
    DrawText("ENTER  RESTART", 535, 485, 16, {185,205,220,255});
}

void Stage3Game::DrawOverlay() const {
    if (flow == Flow::Intro) {
        DrawRectangle(0, 0, 1280, 720, {4,6,14,225});
        DrawText("DISTRICT FURY", 465, 110, 42, {230,235,245,255});
        DrawText("STAGE 3 // ASTRA TOWER", 440, 168, 24, {255,90,190,255});
        DrawText(storyMessage, 230, 230, 18, {205,215,230,255});
        DrawText("1 EASY    2 NORMAL    3 HARD", 475, 305, 17, {170,195,215,255});
        DrawText(TextFormat("SELECTED: %s", DifficultyText()), 545, 340, 18, {80,220,255,255});
        DrawText("ENTER  START", 535, 440, 20, RAYWHITE);
        DrawText("WASD MOVE   J PUNCH   K KICK   L ENERGY   SHIFT DASH   SPACE RAGE", 265, 555, 13, {165,180,200,255});
    } else if (flow == Flow::Gatekeeper) {
        DrawRectangle(280, 270, 720, 120, {5,8,16,225});
        DrawText("GATEKEEPER", 530, 292, 25, {255,120,190,255});
        DrawText(storyMessage, 350, 335, 15, {210,220,235,255});
    } else if (flow == Flow::BossIntro) {
        DrawRectangle(170, 270, 940, 130, {4,6,15,235});
        DrawText("TITAN-X // ASTRA CORE", 430, 292, 28, {255,80,190,255});
        DrawText(storyMessage, 305, 340, 16, {210,220,235,255});
        DrawText("READ THE TELEGRAPH. BREAK THE SHIELD WINDOW.", 400, 370, 13, {90,220,255,255});
    } else if (flow == Flow::Pause) {
        DrawRectangle(0, 0, 1280, 720, {0,0,0,175});
        DrawText("PAUSED", 550, 280, 38, RAYWHITE);
        DrawText("ENTER / P / ESC  RESUME", 485, 340, 16, {190,205,220,255});
    } else if (flow == Flow::GameOver) {
        DrawRectangle(0, 0, 1280, 720, {15,3,10,220});
        DrawText("ASTRA TOWER // FAILURE", 380, 210, 32, {255,75,125,255});
        DrawText("ENTER  RESTART", 515, 350, 18, RAYWHITE);
    } else if (flow == Flow::Clear) {
        DrawClear();
    }
}

void Stage3Game::Draw() const {
    BeginDrawing();
    ClearBackground({5,7,14,255});
    DrawWorld();
    DrawHUD();
    DrawOverlay();
    EndDrawing();
}

}
