#include "game/ProductionGame.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace district_fury {
namespace {
constexpr float kPi = 3.14159265359f;

struct Spawn { float x; float y; StreetEnemyType type; };

const Spawn kWave1[] = {
    {850, 570, StreetEnemyType::Punk}, {1030, 535, StreetEnemyType::Punk}, {1190, 610, StreetEnemyType::Punk}
};
const Spawn kWave2[] = {
    {1450, 560, StreetEnemyType::Punk}, {1630, 520, StreetEnemyType::Charger}, {1800, 615, StreetEnemyType::Punk}, {1950, 545, StreetEnemyType::Charger}
};
const Spawn kWave3[] = {
    {2350, 610, StreetEnemyType::Brute}, {2530, 530, StreetEnemyType::Punk}, {2680, 585, StreetEnemyType::Punk}, {2840, 525, StreetEnemyType::Brute}
};
const Spawn kWave4[] = {
    {3300, 560, StreetEnemyType::Enforcer}, {3470, 515, StreetEnemyType::Charger}, {3650, 620, StreetEnemyType::Punk}, {3820, 550, StreetEnemyType::Charger}, {4000, 600, StreetEnemyType::Enforcer}
};

float Dist(Vector3D a, Vector3D b) {
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return std::sqrt(dx * dx + dy * dy);
}

Color LerpAlpha(Color c, float a) {
    c.a = static_cast<unsigned char>(std::clamp(a, 0.0f, 1.0f) * 255.0f);
    return c;
}
}

ProductionGame::ProductionGame()
    : flow(ProductionFlow::Menu), difficulty(Difficulty::Normal), cameraX(640), introTimer(0),
      encounterBannerTimer(0), bossBannerTimer(0), stageTime(0), hitstop(0), shake(0), score(0),
      comboTimer(0), combo(0), maxCombo(0), defeated(0), totalEnemies(0), damageTaken(0),
      stageReward(0), xp(0), coins(0), gems(0), level(1), stageComplete(false), bossSpawned(false),
      saveLoaded(false), arenaLocked(false), currentWave(0), waveDefeated(0), savePath("district_fury_save.dat") {
    boss = {{0, 575, 0}, 800, 800, 1, 0, 1.4f, 0, 0, false, false, BossAttack::None};
}

ProductionGame::~ProductionGame() = default;

void ProductionGame::Init() {
    scene.Init();
    LoadSave();
    ResetRun();
    flow = ProductionFlow::Menu;
}

void ProductionGame::ResetRun() {
    player.Reset();
    enemies.clear();
    projectiles.clear();
    particles.clear();
    stageTime = 0;
    score = 0;
    combo = 0;
    maxCombo = 0;
    defeated = 0;
    totalEnemies = 0;
    damageTaken = 0;
    stageReward = 0;
    arenaLocked = false;
    currentWave = 0;
    waveDefeated = 0;
    bossSpawned = false;
    stageComplete = false;
    hitstop = 0;
    shake = 0;
    cameraX = kViewportWidth * 0.5f;
    boss = {{0, 575, 0}, 800, 800, 1, 0, 1.4f, 0, 0, false, false, BossAttack::None};
    BuildStage();
}

void ProductionGame::BuildStage() {
    // Spawn definitions are deliberately authored in waves rather than as one giant enemy list.
    // This keeps the stage readable and gives the arena controller authority over pacing.
    for (const auto& s : kWave1) { StreetEnemy e; e.Init({s.x, s.y, 0}, s.type); enemies.push_back(e); }
    for (const auto& s : kWave2) { StreetEnemy e; e.Init({s.x, s.y, 0}, s.type); enemies.push_back(e); }
    for (const auto& s : kWave3) { StreetEnemy e; e.Init({s.x, s.y, 0}, s.type); enemies.push_back(e); }
    for (const auto& s : kWave4) { StreetEnemy e; e.Init({s.x, s.y, 0}, s.type); enemies.push_back(e); }
    totalEnemies = static_cast<int>(enemies.size());
    ApplyDifficulty();
}

void ProductionGame::ApplyDifficulty() {
    // Enemy classes own their normal combat identity. Difficulty changes pressure through HP.
    const float multiplier = difficulty == Difficulty::Easy ? 0.78f : difficulty == Difficulty::Hard ? 1.22f : 1.0f;
    for (auto& e : enemies) {
        e.hp = std::max(1, static_cast<int>(std::round(e.hp * multiplier)));
        e.maxHp = e.hp;
    }
    boss.maxHp = difficulty == Difficulty::Easy ? 680 : difficulty == Difficulty::Hard ? 960 : 800;
    boss.hp = boss.maxHp;
}

void ProductionGame::SpawnWave(int wave) {
    currentWave = wave;
    waveDefeated = 0;
    arenaLocked = true;
    encounterBannerTimer = 1.8f;
    for (auto& e : enemies) e.active = false;
    const float start = wave == 1 ? 0.0f : wave == 2 ? 1200.0f : wave == 3 ? 2150.0f : 3150.0f;
    const float end = wave == 1 ? 1350.0f : wave == 2 ? 2150.0f : wave == 3 ? 3150.0f : 4400.0f;
    for (auto& e : enemies) {
        if (e.position.x >= start && e.position.x < end) e.active = true;
    }
}

void ProductionGame::ActivateNearbyEnemies() {
    if (currentWave == 0) {
        if (player.position.x > 430) SpawnWave(1);
        return;
    }
    // Do not leak later waves into the current arena.
    if (arenaLocked) return;
}

bool ProductionGame::ActiveWaveCleared() const {
    bool found = false;
    for (const auto& e : enemies) {
        const bool belongs = currentWave == 1 ? (e.position.x < 1350) :
            currentWave == 2 ? (e.position.x >= 1200 && e.position.x < 2150) :
            currentWave == 3 ? (e.position.x >= 2150 && e.position.x < 3150) :
            (e.position.x >= 3150 && e.position.x < 4400);
        if (belongs) { found = true; if (!e.IsDefeated()) return false; }
    }
    return found;
}

bool ProductionGame::AllStreetEnemiesCleared() const {
    return std::all_of(enemies.begin(), enemies.end(), [](const StreetEnemy& e) { return e.IsDefeated(); });
}

void ProductionGame::SpawnImpact(Vector3D pos, Color color, bool heavy) {
    const int count = heavy ? 26 : 14;
    for (int i = 0; i < count; ++i) {
        const float a = (static_cast<float>(i) / count) * 2.0f * kPi;
        const float speed = static_cast<float>(GetRandomValue(80, heavy ? 340 : 230));
        particles.push_back({pos, {std::cos(a) * speed, std::sin(a) * speed}, heavy ? 0.48f : 0.28f, heavy ? 0.48f : 0.28f,
                             static_cast<float>(GetRandomValue(3, heavy ? 9 : 6)), color});
    }
}

void ProductionGame::SpawnEnergyProjectile() {
    const float direction = player.facing == Facing::Right ? 1.0f : -1.0f;
    projectiles.push_back({{player.position.x + direction * 72.0f, player.position.y - 72.0f, 0},
                           direction * 720.0f, 0.85f, 18.0f,
                           player.isRageMode ? 30 : 22, true});
    SpawnImpact({player.position.x + direction * 55.0f, player.position.y - 72.0f, 0}, {0, 210, 255, 255}, true);
}

void ProductionGame::UpdateProjectiles(float dt) {
    for (auto& p : projectiles) {
        if (!p.active) continue;
        p.position.x += p.velocity * dt;
        p.life -= dt;
        if (p.life <= 0) { p.active = false; continue; }

        CombatBox box{p.position.x - p.radius, p.position.y - p.radius, p.radius * 2, p.radius * 2};
        for (auto& e : enemies) {
            if (!e.active || e.IsDefeated()) continue;
            if (!box.Intersects(e.GetHurtbox())) continue;
            e.TakeDamage(p.damage, {p.velocity > 0 ? 380.0f : -380.0f, 0, 0});
            p.active = false;
            ++combo;
            comboTimer = 1.0f;
            maxCombo = std::max(maxCombo, combo);
            score += 120 + combo * 8;
            SpawnImpact(p.position, {40, 205, 255, 255}, true);
            hitstop = std::max(hitstop, 0.10f);
            shake = std::max(shake, 0.12f);
            break;
        }
        if (flow == ProductionFlow::Boss && !boss.defeated && p.active) {
            CombatBox bossBox{boss.position.x - 55, boss.position.y - 125, 110, 125};
            if (box.Intersects(bossBox) && boss.invulnerability <= 0) {
                boss.hp = std::max(0, boss.hp - p.damage);
                boss.invulnerability = 0.12f;
                p.active = false;
                ++combo; comboTimer = 1.0f; maxCombo = std::max(maxCombo, combo);
                score += 250 + combo * 12;
                SpawnImpact(p.position, {50, 210, 255, 255}, true);
                hitstop = std::max(hitstop, 0.12f); shake = std::max(shake, 0.16f);
            }
        }
    }
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(), [](const EnergyProjectile& p) { return !p.active; }), projectiles.end());
}

void ProductionGame::HandlePlayerHits() {
    // L is a projectile attack. Do not also apply the old invisible rectangular energy hitbox.
    if (!player.AttackIsActive() || player.hasHit || player.attackType == AttackType::Energy) return;
    const CombatBox attack = player.GetAttackHitbox();
    int best = -1; float nearest = 1e9f;
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        auto& e = enemies[i];
        if (!e.active || e.IsDefeated() || !attack.Intersects(e.GetHurtbox())) continue;
        const float d = Dist(player.position, e.position);
        if (d < nearest) { nearest = d; best = static_cast<int>(i); }
    }
    if (best < 0) return;
    auto& e = enemies[static_cast<std::size_t>(best)];
    const bool heavy = player.attackType == AttackType::Kick || player.comboStep >= 2;
    const int damage = player.GetAttackDamage() + (player.isRageMode ? 5 : 0);
    const float dir = player.facing == Facing::Right ? 1.0f : -1.0f;
    e.TakeDamage(damage, {dir * player.GetAttackKnockback(), 0, 0});
    player.hasHit = true;
    ++combo; comboTimer = 1.0f; maxCombo = std::max(maxCombo, combo);
    score += damage * 10 + combo * 6;
    SpawnImpact({e.position.x, e.position.y - 68, 0}, heavy ? Color{255, 150, 45, 255} : Color{255, 235, 150, 255}, heavy);
    hitstop = std::max(hitstop, heavy ? 0.105f : 0.065f);
    shake = std::max(shake, heavy ? 0.14f : 0.07f);
}

void ProductionGame::HandleEnemyHits() {
    if (player.state == PlayerState::Defeat) return;
    for (auto& e : enemies) {
        if (!e.active || e.IsDefeated() || e.hasHit || !e.AttackIsActive()) continue;
        if (!e.GetAttackHitbox().Intersects(player.GetHurtbox())) continue;
        const int before = player.hp;
        player.TakeDamage(static_cast<int>(e.attackDamage));
        damageTaken += before - player.hp;
        e.hasHit = true;
        combo = 0; comboTimer = 0;
        SpawnImpact({player.position.x, player.position.y - 60, 0}, {255, 90, 75, 255}, false);
        hitstop = std::max(hitstop, 0.085f);
        shake = std::max(shake, 0.12f);
        break;
    }
}

void ProductionGame::HandleBossHits() {
    if (flow != ProductionFlow::Boss || boss.defeated || boss.invulnerability > 0) return;
    if (!player.AttackIsActive() || player.hasHit || player.attackType == AttackType::Energy) return;
    CombatBox hit = player.GetAttackHitbox();
    CombatBox target{boss.position.x - 65, boss.position.y - 135, 130, 135};
    if (!hit.Intersects(target)) return;
    const int damage = player.GetAttackDamage() + (player.isRageMode ? 8 : 0);
    boss.hp = std::max(0, boss.hp - damage);
    boss.invulnerability = 0.10f;
    player.hasHit = true;
    ++combo; comboTimer = 1.0f; maxCombo = std::max(maxCombo, combo);
    score += 220 + combo * 12;
    SpawnImpact({boss.position.x, boss.position.y - 90, 0}, {255, 175, 55, 255}, true);
    hitstop = std::max(hitstop, 0.115f); shake = std::max(shake, 0.18f);
    if (boss.hp <= 0) DefeatBoss();
}

void ProductionGame::UpdateCombat(float dt) {
    if (comboTimer > 0) comboTimer -= dt; else combo = 0;
    player.Update(dt);

    static PlayerState previousState = PlayerState::Idle;
    static AttackType previousAttack = AttackType::None;
    if (player.state == PlayerState::Attack && player.attackType == AttackType::Energy &&
        (previousState != PlayerState::Attack || previousAttack != AttackType::Energy)) {
        SpawnEnergyProjectile();
    }
    previousState = player.state;
    previousAttack = player.attackType;

    if (currentWave == 0) ActivateNearbyEnemies();
    for (auto& e : enemies) if (e.active) e.Update(dt, player);
    HandlePlayerHits();
    UpdateProjectiles(dt);
    HandleEnemyHits();

    if (currentWave > 0 && ActiveWaveCleared()) {
        ++waveDefeated;
        arenaLocked = false;
        if (currentWave < 4) {
            currentWave++;
            const float trigger = currentWave == 2 ? 1380 : currentWave == 3 ? 2180 : 3180;
            if (player.position.x > trigger - 300) SpawnWave(currentWave);
        } else if (AllStreetEnemiesCleared() && player.position.x > 4250) {
            EnterBoss();
            return;
        }
    }

    if (currentWave > 0 && !arenaLocked) {
        const float trigger = currentWave == 1 ? 1380 : currentWave == 2 ? 2180 : currentWave == 3 ? 3180 : 4250;
        if (player.position.x > trigger && currentWave < 4) SpawnWave(currentWave + 1);
        if (currentWave == 4 && player.position.x > 4300 && AllStreetEnemiesCleared()) EnterBoss();
    }

    if (player.state == PlayerState::Defeat) flow = ProductionFlow::GameOver;
}

void ProductionGame::EnterBoss() {
    if (bossSpawned) return;
    bossSpawned = true;
    arenaLocked = true;
    boss.position = {4850, 575, 0};
    boss.hp = boss.maxHp;
    boss.phase = 1;
    boss.stateTimer = 2.2f;
    boss.attackTimer = 1.0f;
    boss.attackElapsed = 0;
    boss.invulnerability = 0;
    boss.defeated = false;
    boss.attack = BossAttack::None;
    flow = ProductionFlow::BossIntro;
    bossBannerTimer = 2.4f;
}

void ProductionGame::UpdateBoss(float dt) {
    if (boss.invulnerability > 0) boss.invulnerability -= dt;
    if (flow == ProductionFlow::BossIntro) {
        boss.stateTimer -= dt;
        player.position.x = std::min(player.position.x, boss.position.x - 260.0f);
        if (boss.stateTimer <= 0) flow = ProductionFlow::Boss;
        return;
    }
    if (boss.defeated) return;

    const float hpRatio = static_cast<float>(boss.hp) / boss.maxHp;
    const int wantedPhase = hpRatio <= 0.35f ? 3 : hpRatio <= 0.70f ? 2 : 1;
    if (wantedPhase != boss.phase) {
        boss.phase = wantedPhase;
        boss.stateTimer = 0.7f;
        boss.attack = BossAttack::Frenzy;
        SpawnImpact(boss.position, {255, 70, 45, 255}, true);
        shake = 0.25f;
    }

    boss.attackTimer -= dt;
    boss.attackElapsed += dt;
    const float dx = player.position.x - boss.position.x;
    const float dy = player.position.y - boss.position.y;
    const float absX = std::abs(dx);

    if (boss.attack == BossAttack::None && boss.attackTimer <= 0) {
        const int pick = GetRandomValue(0, boss.phase == 1 ? 2 : 3);
        boss.attack = pick == 0 ? BossAttack::ChainSwing : pick == 1 ? BossAttack::GroundSmash : pick == 2 ? BossAttack::Charge : BossAttack::Frenzy;
        boss.attackElapsed = 0;
        boss.attackTimer = boss.phase == 3 ? 1.25f : boss.phase == 2 ? 1.55f : 1.9f;
    }

    if (boss.attack == BossAttack::None) {
        const float speed = boss.phase == 3 ? 115 : boss.phase == 2 ? 92 : 72;
        if (absX > 190) boss.position.x += (dx > 0 ? 1 : -1) * speed * dt;
        if (std::abs(dy) > 35) boss.position.y += (dy > 0 ? 1 : -1) * speed * 0.55f * dt;
    }

    const float telegraph = boss.attack == BossAttack::Charge ? 0.48f : boss.attack == BossAttack::GroundSmash ? 0.55f : 0.32f;
    if (boss.attack != BossAttack::None && boss.attackElapsed >= telegraph) {
        CombatBox hit{};
        bool active = false;
        if (boss.attack == BossAttack::ChainSwing) {
            hit = {boss.position.x - 125, boss.position.y - 105, 250, 95}; active = boss.attackElapsed <= telegraph + 0.22f;
        } else if (boss.attack == BossAttack::GroundSmash) {
            hit = {boss.position.x - 250, boss.position.y - 80, 500, 100}; active = boss.attackElapsed <= telegraph + 0.18f;
        } else if (boss.attack == BossAttack::Charge) {
            boss.position.x += (dx > 0 ? 1 : -1) * 520.0f * dt;
            hit = {boss.position.x - 85, boss.position.y - 105, 170, 105}; active = boss.attackElapsed <= telegraph + 0.42f;
        } else {
            hit = {boss.position.x - 160, boss.position.y - 125, 320, 130}; active = boss.attackElapsed <= telegraph + 0.30f;
        }
        if (active && hit.Intersects(player.GetHurtbox()) && player.state != PlayerState::Hit) {
            const int damage = boss.phase == 3 ? 28 : boss.phase == 2 ? 23 : 19;
            const int before = player.hp;
            player.TakeDamage(damage);
            damageTaken += before - player.hp;
            combo = 0; comboTimer = 0;
            SpawnImpact(player.position, {255, 70, 50, 255}, true);
            hitstop = 0.10f; shake = 0.18f;
        }
    }
    if (boss.attack != BossAttack::None && boss.attackElapsed > (boss.attack == BossAttack::Charge ? 0.95f : 0.82f)) {
        boss.attack = BossAttack::None;
        boss.attackElapsed = 0;
    }

    boss.position.x = std::clamp(boss.position.x, 4550.0f, 5450.0f);
    boss.position.y = std::clamp(boss.position.y, kLaneMinY + 25.0f, kLaneMaxY);
    HandleBossHits();
    if (boss.hp <= 0) DefeatBoss();
}

void ProductionGame::DefeatBoss() {
    if (boss.defeated) return;
    boss.defeated = true;
    boss.attack = BossAttack::None;
    boss.stateTimer = 2.0f;
    score += 5000;
    SpawnImpact(boss.position, {255, 150, 40, 255}, true);
    shake = 0.45f;
    stageReward = 1000 + CalculateRank() * 250;
    flow = ProductionFlow::StageClear;
    stageComplete = true;
    xp += 500;
    coins += stageReward;
    gems += 5;
    SaveProgress();
}

void ProductionGame::FinishStage() {
    stageComplete = true;
    flow = ProductionFlow::StageClear;
    SaveProgress();
}

void ProductionGame::UpdateParticles(float dt) {
    for (auto& p : particles) {
        p.life -= dt;
        p.position.x += p.velocity.x * dt;
        p.position.y += p.velocity.y * dt;
        p.velocity.x *= 0.92f;
        p.velocity.y *= 0.92f;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(), [](const ProductionParticle& p) { return p.life <= 0; }), particles.end());
}

void ProductionGame::Update(float dt) {
    dt = std::min(dt, 0.033f);
    if (IsKeyPressed(KEY_F1)) { difficulty = difficulty == Difficulty::Normal ? Difficulty::Hard : difficulty == Difficulty::Hard ? Difficulty::Easy : Difficulty::Normal; }

    if (flow == ProductionFlow::Menu) {
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_J)) { ResetRun(); flow = ProductionFlow::Intro; introTimer = 1.8f; }
        if (IsKeyPressed(KEY_R)) { ResetSave(); LoadSave(); }
        return;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (flow == ProductionFlow::Combat || flow == ProductionFlow::Boss) flow = ProductionFlow::Pause;
        else if (flow == ProductionFlow::Pause) flow = bossSpawned ? ProductionFlow::Boss : ProductionFlow::Combat;
    }
    if (flow == ProductionFlow::Pause) return;
    if (flow == ProductionFlow::GameOver) { if (IsKeyPressed(KEY_R)) { ResetRun(); flow = ProductionFlow::Intro; introTimer = 1.0f; } if (IsKeyPressed(KEY_Q)) flow = ProductionFlow::Menu; return; }
    if (flow == ProductionFlow::StageClear) { if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_R)) { SaveProgress(); flow = ProductionFlow::Menu; } return; }

    if (hitstop > 0) { hitstop -= dt; return; }
    shake = std::max(0.0f, shake - dt);
    encounterBannerTimer = std::max(0.0f, encounterBannerTimer - dt);
    bossBannerTimer = std::max(0.0f, bossBannerTimer - dt);
    UpdateParticles(dt);

    if (flow == ProductionFlow::Intro) {
        introTimer -= dt;
        if (introTimer <= 0) { flow = ProductionFlow::Combat; SpawnWave(1); }
        return;
    }

    stageTime += dt;
    if (flow == ProductionFlow::Combat) UpdateCombat(dt);
    if (flow == ProductionFlow::BossIntro || flow == ProductionFlow::Boss) UpdateBoss(dt);

    const float minCam = kViewportWidth * 0.5f;
    const float maxCam = kStageEndX - kViewportWidth * 0.5f;
    const float target = std::clamp(player.position.x, minCam, maxCam);
    cameraX += (target - cameraX) * (1.0f - std::pow(0.001f, dt));
}

int ProductionGame::CalculateRank() const {
    float value = 0;
    value += std::max(0.0f, 250.0f - stageTime) * 0.35f;
    value += static_cast<float>(maxCombo) * 12.0f;
    value += static_cast<float>(player.hp) * 1.8f;
    value -= static_cast<float>(damageTaken) * 2.0f;
    if (value >= 400) return 7;
    if (value >= 330) return 6;
    if (value >= 270) return 5;
    if (value >= 215) return 4;
    if (value >= 165) return 3;
    if (value >= 115) return 2;
    if (value >= 70) return 1;
    return 0;
}

int ProductionGame::CalculateScore() const { return static_cast<int>(score + player.hp * 4 + maxCombo * 100); }

const char* ProductionGame::RankText() const {
    static const char* ranks[] = {"D", "C", "B", "A", "S", "SS", "SSS", "SSS"};
    return ranks[CalculateRank()];
}

const char* ProductionGame::DifficultyText() const {
    return difficulty == Difficulty::Easy ? "EASY" : difficulty == Difficulty::Hard ? "HARD" : "NORMAL";
}

void ProductionGame::LoadSave() {
    std::ifstream in(savePath);
    if (!in) return;
    in >> xp >> coins >> gems >> level;
    saveLoaded = true;
}

void ProductionGame::SaveProgress() const {
    std::ofstream out(savePath, std::ios::trunc);
    if (out) out << xp << ' ' << coins << ' ' << gems << ' ' << level << '\n';
}

void ProductionGame::ResetSave() {
    std::ofstream out(savePath, std::ios::trunc);
    if (out) out << "0 0 0 1\n";
}

void ProductionGame::DrawBoss() const {
    if (!bossSpawned || boss.defeated) return;
    const Vector2 p = boss.position.ToScreen();
    const float s = boss.phase == 3 ? 1.15f : boss.phase == 2 ? 1.08f : 1.0f;
    Color body = boss.phase == 3 ? Color{185, 35, 45, 255} : Color{115, 30, 38, 255};
    DrawEllipse(static_cast<int>(p.x), static_cast<int>(p.y), 58*s, 12*s, {0,0,0,160});
    DrawRectangle(static_cast<int>(p.x - 48*s), static_cast<int>(p.y - 120*s), static_cast<int>(96*s), static_cast<int>(105*s), body);
    DrawCircle(static_cast<int>(p.x), static_cast<int>(p.y - 145*s), static_cast<int>(37*s), {40,45,50,255});
    DrawRectangle(static_cast<int>(p.x - 31*s), static_cast<int>(p.y - 151*s), static_cast<int>(62*s), static_cast<int>(12*s), {220,190,70,255});
    DrawLine(static_cast<int>(p.x - 42*s), static_cast<int>(p.y - 100*s), static_cast<int>(p.x - 105*s), static_cast<int>(p.y - 55*s), {170,170,180,255});
    DrawLine(static_cast<int>(p.x + 42*s), static_cast<int>(p.y - 100*s), static_cast<int>(p.x + 105*s), static_cast<int>(p.y - 55*s), {170,170,180,255});
    DrawCircleLines(static_cast<int>(p.x), static_cast<int>(p.y - 145*s), 42*s + std::sin(static_cast<float>(GetTime())*7)*4, boss.phase == 3 ? Color{255,65,50,100} : Color{40,190,255,80});
}

void ProductionGame::DrawArenaLock() const {
    if (!arenaLocked) return;
    const float left = player.position.x - 300;
    const float right = player.position.x + 300;
    for (int x = static_cast<int>(left); x <= static_cast<int>(right); x += 24) {
        DrawRectangle(x, 430, 7, 220, {210,45,55,85});
        DrawLine(x, 430, x + 45, 385, {255,75,65,65});
    }
}

void ProductionGame::DrawWorld() const {
    Camera2D camera{};
    camera.offset = {kViewportWidth*0.5f, kViewportHeight*0.5f};
    camera.target = {cameraX, kViewportHeight*0.5f};
    camera.rotation = 0;
    camera.zoom = 1;
    if (shake > 0) {
        camera.target.x += static_cast<float>(GetRandomValue(-100,100))*shake*8;
        camera.target.y += static_cast<float>(GetRandomValue(-100,100))*shake*5;
    }
    BeginMode2D(camera);
    scene.DrawBackground(cameraX);
    DrawArenaLock();

    struct Item { float y; int id; };
    std::vector<Item> order; order.push_back({player.position.y,-1});
    for (std::size_t i=0;i<enemies.size();++i) if (enemies[i].active) order.push_back({enemies[i].position.y,static_cast<int>(i)});
    std::sort(order.begin(), order.end(), [](const Item&a,const Item&b){return a.y<b.y;});
    for (const auto& i:order) { if(i.id<0) player.Draw(); else enemies[static_cast<std::size_t>(i.id)].Draw(); }
    DrawBoss();

    for (const auto& p:projectiles) {
        const Vector2 s=p.position.ToScreen();
        DrawCircle(static_cast<int>(s.x),static_cast<int>(s.y),p.radius,{30,185,255,70});
        DrawCircle(static_cast<int>(s.x),static_cast<int>(s.y),p.radius*0.62f,{110,235,255,255});
        DrawLine(static_cast<int>(s.x-p.velocity*0.04f),static_cast<int>(s.y),static_cast<int>(s.x),static_cast<int>(s.y),{80,220,255,160});
    }
    for(const auto& p:particles) {
        const Vector2 s=p.position.ToScreen();
        const float a=p.life/p.maxLife;
        DrawCircle(static_cast<int>(s.x),static_cast<int>(s.y),p.size*a,LerpAlpha(p.color,a));
    }
    scene.DrawForeground(cameraX);
    EndMode2D();
}

void ProductionGame::DrawHUD() const {
    DrawRectangle(16,14,480,118,{5,8,12,235});
    DrawText("RAYDEN CRUZ",28,22,22,{185,220,255,255});
    DrawText(TextFormat("HP %d/%d",player.hp,player.maxHp),28,52,16,WHITE);
    DrawRectangle(112,53,240,12,{25,25,30,255});
    DrawRectangle(112,53,static_cast<int>(240.0f*player.hp/player.maxHp),12,{45,170,255,255});
    DrawText(TextFormat("SP %d",player.sp),28,75,15,WHITE);
    DrawRectangle(112,76,180,9,{25,25,30,255});
    DrawRectangle(112,76,static_cast<int>(180.0f*player.sp/player.maxSp),9,{75,220,255,255});
    DrawText(TextFormat("RAGE %d%%",player.rage),305,75,15,player.isRageMode?Color{90,210,255,255}:WHITE);
    DrawRectangle(112,92,180,8,{25,25,30,255});
    DrawRectangle(112,92,static_cast<int>(180.0f*player.rage/player.maxRage),8,{45,130,255,255});
    if(combo>1) DrawText(TextFormat("COMBO x%d",combo),360,52,22,{255,190,65,255});
    DrawText(TextFormat("SCORE %d",CalculateScore()),360,88,16,WHITE);
    DrawText(DifficultyText(),405,110,13,LIGHTGRAY);

    if (bossSpawned && !boss.defeated) {
        DrawRectangle(300,18,680,48,{8,8,10,230});
        DrawText("BRAKK  \"THE CHAIN\"",460,21,22,{255,220,180,255});
        DrawRectangle(350,50,580,12,{35,20,20,255});
        DrawRectangle(350,50,static_cast<int>(580.0f*boss.hp/boss.maxHp),12,{220,50,55,255});
        DrawText(TextFormat("PHASE %d",boss.phase),940,49,15,WHITE);
    }
    if (encounterBannerTimer>0) {
        const char* text=TextFormat("WAVE %d",currentWave);
        DrawText(text,static_cast<int>(640-MeasureText(text,36)/2),190,36,{255,225,175,220});
    }
    if (bossBannerTimer>0) {
        DrawRectangle(210,260,860,130,{8,8,12,235});
        DrawText("BRAKK",545,280,52,{255,75,65,255});
        DrawText("THE CHAIN",500,335,30,WHITE);
    }
}

void ProductionGame::DrawMenu() const {
    DrawRectangle(0,0,1280,720,{6,9,13,255});
    DrawText("DISTRICT",330,120,82,{190,210,225,255});
    DrawText("FURY",455,205,100,{45,170,255,255});
    DrawText("FULL PLAYABLE // DF-008",445,315,18,LIGHTGRAY);
    DrawText("ENTER / J  —  START",480,410,24,WHITE);
    DrawText("F1 — DIFFICULTY",510,450,17,LIGHTGRAY);
    DrawText("R — RESET SAVE",515,478,17,LIGHTGRAY);
    DrawText(TextFormat("LEVEL %d   XP %d   COINS %d   GEMS %d",level,xp,coins,gems),380,560,18,{150,180,205,255});
}

void ProductionGame::DrawPause() const {
    DrawRectangle(0,0,1280,720,{0,0,0,155});
    DrawRectangle(350,190,580,330,{7,10,15,245});
    DrawText("PAUSED",545,235,48,{185,220,255,255});
    DrawText("ESC — RESUME",500,330,22,WHITE);
    DrawText("R — RESTART",510,375,22,WHITE);
    DrawText("Q — MENU",520,420,22,WHITE);
}

void ProductionGame::DrawGameOver() const {
    DrawRectangle(0,0,1280,720,{8,4,7,210});
    DrawText("GAME OVER",440,230,62,{235,55,65,255});
    DrawText("R — RESTART",500,355,25,WHITE);
    DrawText("Q — MENU",520,400,22,LIGHTGRAY);
}

void ProductionGame::DrawStageClear() const {
    DrawRectangle(0,0,1280,720,{5,8,12,245});
    DrawText("STAGE CLEAR",430,100,58,{70,200,255,255});
    DrawText("OLD STEEL YARD",480,170,22,LIGHTGRAY);
    DrawText(TextFormat("TIME        %6.1fs",stageTime),380,250,23,WHITE);
    DrawText(TextFormat("ENEMIES     %6d",defeated),380,290,23,WHITE);
    DrawText(TextFormat("DAMAGE      %6d",damageTaken),380,330,23,WHITE);
    DrawText(TextFormat("MAX COMBO   %6d",maxCombo),380,370,23,WHITE);
    DrawText(TextFormat("SCORE       %6d",CalculateScore()),380,410,23,WHITE);
    DrawText(TextFormat("RANK        %s",RankText()),380,475,44,{255,205,70,255});
    DrawText(TextFormat("REWARD      %d COINS  +5 GEMS",stageReward),380,535,20,{170,220,255,255});
    DrawText("ENTER / R — CONTINUE",455,625,22,WHITE);
}

void ProductionGame::Draw() const {
    if (flow == ProductionFlow::Menu) { DrawMenu(); return; }
    DrawWorld();
    if (flow == ProductionFlow::Intro) {
        DrawRectangle(0,0,1280,720,{5,8,12,155});
        DrawText("OLD STEEL YARD",445,290,46,{190,220,235,255});
        DrawText("DISTRICT 01",535,345,20,LIGHTGRAY);
    }
    if (flow == ProductionFlow::Combat || flow == ProductionFlow::BossIntro || flow == ProductionFlow::Boss) DrawHUD();
    if (flow == ProductionFlow::Pause) { DrawHUD(); DrawPause(); }
    if (flow == ProductionFlow::GameOver) { DrawHUD(); DrawGameOver(); }
    if (flow == ProductionFlow::StageClear) DrawStageClear();
}

} // namespace district_fury
