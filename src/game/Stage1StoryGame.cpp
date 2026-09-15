#include "game/Stage1StoryGame.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace district_fury {
namespace {
constexpr float kStageEnd = 6000.0f;
constexpr float kLaneMin = 505.0f;
constexpr float kLaneMax = 625.0f;
constexpr float kPi = 3.14159265359f;

struct Spawn { float x; float y; StreetEnemyType type; };
const Spawn kScenarioWaves[4][5] = {
    {{620,570,StreetEnemyType::Punk},{790,535,StreetEnemyType::Punk},{960,610,StreetEnemyType::Charger},{1120,545,StreetEnemyType::Punk},{0,0,StreetEnemyType::Punk}},
    {{1620,570,StreetEnemyType::Punk},{1770,525,StreetEnemyType::Charger},{1930,610,StreetEnemyType::Punk},{2090,545,StreetEnemyType::Brute},{2240,585,StreetEnemyType::Charger}},
    {{3020,575,StreetEnemyType::Charger},{3180,525,StreetEnemyType::Brute},{3350,610,StreetEnemyType::Punk},{3520,545,StreetEnemyType::Enforcer},{3670,590,StreetEnemyType::Charger}},
    {{4420,575,StreetEnemyType::Enforcer},{4580,525,StreetEnemyType::Charger},{4740,610,StreetEnemyType::Brute},{4900,545,StreetEnemyType::Enforcer},{5060,590,StreetEnemyType::Punk}}
};

float Dist(Vector3D a, Vector3D b) { const float dx=a.x-b.x, dy=a.y-b.y; return std::sqrt(dx*dx+dy*dy); }
Color WithAlpha(Color c,float a){c.a=(unsigned char)(std::clamp(a,0.0f,1.0f)*255);return c;}
}

Stage1StoryGame::Stage1StoryGame() = default;

void Stage1StoryGame::Init() {
    LoadSave();
    ResetRun();
    flow = StoryFlow::Menu;
}

void Stage1StoryGame::ResetRun() {
    player.Reset(); player.position={180,585,0};
    enemies.clear(); projectiles.clear(); particles.clear();
    scenario=1; wave=0; combo=0; maxCombo=0; defeated=0; damageTaken=0; score=0; stageTime=0; comboTimer=0; hitstop=0; shake=0;
    bannerTimer=0; transitionTimer=0; storyTimer=0; arenaLocked=false; scenarioBossSpawned=false; finalBossSpawned=false; stageComplete=false;
    cameraX=640; boss=StoryBoss{}; boss.position={5500,575,0}; storyMessage="Rayden enters the Slum District. Brakk's chain crew controls the route to the steel yard.";
    BuildScenario(1);
}

int Stage1StoryGame::ScenarioStartX() const { return scenario==1?180:scenario==2?1500:scenario==3?2900:4300; }
int Stage1StoryGame::ScenarioEndX() const { return scenario==1?1450:scenario==2?2850:scenario==3?4250:5900; }
const char* Stage1StoryGame::ScenarioName() const {
    switch(scenario){case 1:return "SLUM DISTRICT // BLOCK 17";case 2:return "OLD MARKET // CANAL LINE";case 3:return "STEEL GATE // FREIGHT ROUTE";default:return "CHAIN YARD // BRakk'S TERRITORY";}
}
const char* Stage1StoryGame::ScenarioObjective() const {
    switch(scenario){case 1:return "Break the street blockade.";case 2:return "Cut through the market route.";case 3:return "Take the freight gate.";default:return "Reach Brakk's chain yard.";}
}
const char* Stage1StoryGame::DifficultyText() const { return difficulty==StoryDifficulty::Easy?"EASY":difficulty==StoryDifficulty::Hard?"HARD":"NORMAL"; }

void Stage1StoryGame::ApplyDifficulty() {
    const float hpMult=difficulty==StoryDifficulty::Easy?.82f:difficulty==StoryDifficulty::Hard?1.20f:1.f;
    const float dmgMult=difficulty==StoryDifficulty::Easy?.82f:difficulty==StoryDifficulty::Hard?1.18f:1.f;
    for(auto&e:enemies){e.hp=std::max(1,(int)std::round(e.hp*hpMult));e.maxHp=e.hp;e.attackDamage=std::max(1,(int)std::round(e.attackDamage*dmgMult));}
    boss.maxHp=difficulty==StoryDifficulty::Easy?1000:difficulty==StoryDifficulty::Hard?1450:1200; boss.hp=boss.maxHp;
}

void Stage1StoryGame::BuildScenario(int id) {
    enemies.clear(); wave=0; scenarioBossSpawned=false; arenaLocked=false;
    const int index=id-1;
    for(const auto&s:kScenarioWaves[index]) if(s.x>0){StreetEnemy e;e.Init({s.x,s.y,0},s.type);e.active=false;enemies.push_back(e);}
    ApplyDifficulty();
}

void Stage1StoryGame::SpawnWave(int id) {
    wave=id; arenaLocked=true; bannerTimer=1.5f;
    for(auto&e:enemies)e.active=false;
    for(auto&e:enemies)e.active=true;
}

void Stage1StoryGame::SpawnScenarioBoss() {
    scenarioBossSpawned=true; arenaLocked=true; flow=StoryFlow::SubBossIntro; bannerTimer=2.0f;
    enemies.clear(); StreetEnemy bossEnemy;
    const StreetEnemyType type=scenario==1?StreetEnemyType::Brute:scenario==2?StreetEnemyType::Enforcer:StreetEnemyType::ArmoredGuard;
    const float x=(float)(ScenarioEndX()-170);
    bossEnemy.Init({x,575,0},type); bossEnemy.active=true;
    const int hpBonus=scenario==1?150:scenario==2?260:380;
    bossEnemy.hp+=hpBonus; bossEnemy.maxHp=bossEnemy.hp; bossEnemy.attackDamage+=scenario*4;
    enemies.push_back(bossEnemy);
    storyMessage=scenario==1?"BLOCK 17 LIEUTENANT: the chain crew's gatekeeper refuses to retreat.":scenario==2?"MARKET ENFORCER: the route boss seals the canal crossing.":"STEEL GATE GUARD: the freight commander protects Brakk's supply line.";
}

void Stage1StoryGame::AdvanceScenario() {
    if(scenario>=4){EnterFinalBoss();return;}
    ++scenario; wave=0; scenarioBossSpawned=false; arenaLocked=false; transitionTimer=2.4f; storyTimer=2.4f;
    flow=StoryFlow::ScenarioClear;
    if(scenario==2) storyMessage="The market line feeds the old freight route. Rayden follows the chain symbols toward the steel gate.";
    if(scenario==3) storyMessage="The freight route is the gang's lifeline. Beyond the gate is Brakk's private yard.";
    if(scenario==4) storyMessage="The chain symbols converge here. Brakk is waiting at the end of the yard.";
}

bool Stage1StoryGame::ScenarioWaveCleared() const {
    if(enemies.empty())return false;
    return std::all_of(enemies.begin(),enemies.end(),[](const StreetEnemy&e){return e.IsDefeated();});
}
bool Stage1StoryGame::AllCurrentEnemiesDefeated() const { return ScenarioWaveCleared(); }

void Stage1StoryGame::SpawnImpact(Vector3D pos,Color color,bool heavy){
    const int n=heavy?28:14; for(int i=0;i<n;++i){float a=(float)i/n*2*kPi;float s=(float)GetRandomValue(70,heavy?340:220);particles.push_back({pos,{std::cos(a)*s,std::sin(a)*s},heavy?.46f:.26f,heavy?.46f:.26f,(float)GetRandomValue(3,heavy?9:6),color});}
}

void Stage1StoryGame::SpawnEnergyProjectile(){float d=player.facing==Facing::Right?1.f:-1.f;projectiles.push_back({{player.position.x+d*70,player.position.y-70,0},d*760,.9f,20,player.isRageMode?32:23,true,false});SpawnImpact({player.position.x+d*50,player.position.y-70,0},{50,215,255,255},true);}

void Stage1StoryGame::SpawnBossPower(){
    const float d=player.position.x>=boss.position.x?-1.f:1.f;
    const int dmg=boss.phase==3?32:boss.phase==2?27:22;
    projectiles.push_back({{boss.position.x+d*85,boss.position.y-72,0},d*650,1.5f,24,dmg,true,true});
    SpawnImpact({boss.position.x+d*55,boss.position.y-72,0},{255,80,55,255},true);
}

void Stage1StoryGame::UpdateProjectiles(float dt){
    for(auto&p:projectiles){if(!p.active)continue;p.position.x+=p.velocity*dt;p.life-=dt;if(p.life<=0){p.active=false;continue;}
        CombatBox box{p.position.x-p.radius,p.position.y-p.radius,p.radius*2,p.radius*2};
        if(p.fromBoss){if(box.Intersects(player.GetHurtbox())&&player.state!=PlayerState::Defeat&&player.dashInvulnerability<=0){const int before=player.hp;player.TakeDamage(p.damage);damageTaken+=before-player.hp;p.active=false;combo=0;comboTimer=0;SpawnImpact(player.position,{255,70,50,255},true);hitstop=.10f;shake=.18f;}}
        else{
            for(auto&e:enemies)if(e.active&&!e.IsDefeated()&&box.Intersects(e.GetHurtbox())){e.TakeDamage(p.damage,{p.velocity>0?420.f:-420.f,0,0});p.active=false;++combo;comboTimer=1;maxCombo=std::max(maxCombo,combo);score+=150+combo*8;SpawnImpact(p.position,{40,215,255,255},true);hitstop=.10f;shake=.13f;break;}
            if(flow==StoryFlow::Boss&&!boss.defeated&&p.active){CombatBox bb{boss.position.x-70,boss.position.y-140,140,140};if(box.Intersects(bb)&&boss.invulnerability<=0){int dmg=p.damage;if(boss.blocking)dmg=std::max(1,dmg/5);boss.hp=std::max(0,boss.hp-dmg);boss.invulnerability=.12f;p.active=false;++combo;comboTimer=1;maxCombo=std::max(maxCombo,combo);score+=260+combo*12;SpawnImpact(p.position,boss.blocking?Color{120,180,220,255}:Color{255,175,55,255},true);hitstop=.12f;shake=.16f;}}
        }
    }
    projectiles.erase(std::remove_if(projectiles.begin(),projectiles.end(),[](const StoryProjectile&p){return !p.active;}),projectiles.end());
}

void Stage1StoryGame::HandlePlayerHits(){
    if(!player.AttackIsActive()||player.hasHit||player.attackType==AttackType::Energy)return;
    CombatBox hit=player.GetAttackHitbox();
    for(auto&e:enemies)if(e.active&&!e.IsDefeated()&&hit.Intersects(e.GetHurtbox())){const bool heavy=player.attackType==AttackType::Kick||player.comboStep>=2;int dmg=player.GetAttackDamage()+(player.isRageMode?5:0);float d=player.facing==Facing::Right?1.f:-1.f;e.TakeDamage(dmg,{d*player.GetAttackKnockback(),0,0});player.hasHit=true;++combo;comboTimer=1;maxCombo=std::max(maxCombo,combo);score+=dmg*10+combo*7;SpawnImpact({e.position.x,e.position.y-68},heavy?Color{255,150,45,255}:Color{255,235,150,255},heavy);hitstop=heavy?.105f:.065f;shake=heavy?.14f:.07f;break;}
}

void Stage1StoryGame::HandleEnemyHits(){
    if(player.state==PlayerState::Defeat)return; int attackers=0;
    for(auto&e:enemies)if(e.active&&!e.IsDefeated()&&e.AttackIsActive()){if(++attackers>2)continue;if(!e.hasHit&&e.GetAttackHitbox().Intersects(player.GetHurtbox())){int before=player.hp;player.TakeDamage(e.attackDamage);damageTaken+=before-player.hp;e.hasHit=true;combo=0;comboTimer=0;SpawnImpact(player.position,{255,80,70,255},false);hitstop=.085f;shake=.12f;}}
}

void Stage1StoryGame::HandleBossHits(){
    if(flow!=StoryFlow::Boss||boss.defeated||boss.invulnerability>0||!player.AttackIsActive()||player.hasHit||player.attackType==AttackType::Energy)return;
    CombatBox hit=player.GetAttackHitbox();CombatBox target{boss.position.x-78,boss.position.y-145,156,145};if(!hit.Intersects(target))return;
    int dmg=player.GetAttackDamage()+(player.isRageMode?8:0);
    if(boss.blocking){dmg=std::max(1,dmg/5);player.velocity.x+=(player.facing==Facing::Right?-120.f:120.f);SpawnImpact({boss.position.x,boss.position.y-95},{110,180,220,255},true);}
    else SpawnImpact({boss.position.x,boss.position.y-95},{255,175,55,255},true);
    boss.hp=std::max(0,boss.hp-dmg);boss.invulnerability=.10f;player.hasHit=true;++combo;comboTimer=1;maxCombo=std::max(maxCombo,combo);score+=240+combo*12;hitstop=.11f;shake=.18f;if(boss.hp<=0)DefeatFinalBoss();
}

void Stage1StoryGame::UpdateCombat(float dt){
    if(comboTimer>0)comboTimer-=dt;else combo=0;
    player.Update(dt);
    static PlayerState previousState=PlayerState::Idle; static AttackType previousAttack=AttackType::None;
    if(player.state==PlayerState::Attack&&player.attackType==AttackType::Energy&&(previousState!=PlayerState::Attack||previousAttack!=AttackType::Energy))SpawnEnergyProjectile();
    previousState=player.state;previousAttack=player.attackType;

    if(wave==0)SpawnWave(1);
    int activeAttackers=0;
    for(auto&e:enemies)if(e.active){if(e.state==StreetEnemyState::Attack)++activeAttackers;if(e.state!=StreetEnemyState::Attack&&activeAttackers>=2){e.Update(std::min(dt,.016f),player);}else e.Update(dt,player);}
    HandlePlayerHits();UpdateProjectiles(dt);HandleEnemyHits();

    const float left=(float)ScenarioStartX(), right=arenaLocked?(float)(ScenarioEndX()-170):(float)ScenarioEndX();
    player.position.x=std::clamp(player.position.x,left,right);player.position.y=std::clamp(player.position.y,kLaneMin,kLaneMax);
    if(AllCurrentEnemiesDefeated()&&!scenarioBossSpawned){SpawnScenarioBoss();return;}
    if(scenarioBossSpawned&&AllCurrentEnemiesDefeated()){AdvanceScenario();return;}
    if(player.state==PlayerState::Defeat)flow=StoryFlow::GameOver;
}

void Stage1StoryGame::EnterFinalBoss(){
    finalBossSpawned=true;arenaLocked=true;flow=StoryFlow::BossIntro;bannerTimer=2.8f;boss=StoryBoss{};boss.position={5480,575,0};boss.maxHp=difficulty==StoryDifficulty::Easy?1000:difficulty==StoryDifficulty::Hard?1450:1200;boss.hp=boss.maxHp;boss.phase=1;boss.attackTimer=1.1f;boss.powerTimer=2.0f;boss.blockTimer=.8f;storyMessage="BRAKK // THE CHAIN: the gang's enforcer finally steps into the yard.";
}

void Stage1StoryGame::UpdateBoss(float dt){
    if(boss.invulnerability>0)boss.invulnerability-=dt;
    if(boss.blockTimer>0)boss.blockTimer-=dt;
    if(boss.powerTimer>0)boss.powerTimer-=dt;
    if(flow==StoryFlow::BossIntro){bannerTimer-=dt;player.position.x=std::min(player.position.x,boss.position.x-270);if(bannerTimer<=0)flow=StoryFlow::Boss;return;}
    if(boss.defeated)return;
    const float ratio=(float)boss.hp/boss.maxHp;const int phase=ratio<=.34f?3:ratio<=.68f?2:1;
    if(phase!=boss.phase){boss.phase=phase;boss.attack=StoryBossAttack::Frenzy;boss.attackElapsed=0;boss.blockTimer=.65f;boss.blocking=true;boss.powerTimer=.4f;SpawnImpact(boss.position,{255,70,45,255},true);shake=.25f;}

    // Brakk reads the player's attack state and periodically guards, so mashing into him is intentionally unsafe.
    const bool playerThreat=player.AttackIsActive()||player.attackType==AttackType::Energy;
    if(boss.blockTimer<=0&&boss.attack==StoryBossAttack::None){
        const int guardChance=boss.phase==1?24:boss.phase==2?34:46;
        if(playerThreat&&GetRandomValue(0,99)<guardChance){boss.blockTimer=boss.phase==3?.72f:.58f;boss.blocking=true;}
        else boss.blocking=false;
    }
    if(boss.blocking){boss.attackElapsed=0;boss.position.x+=std::clamp(player.position.x-boss.position.x,-45.f,45.f)*dt; if(boss.blockTimer<=0){boss.blocking=false;boss.attackTimer=.15f;} HandleBossHits();return;}

    boss.attackTimer-=dt;boss.attackElapsed+=dt;
    const float dx=player.position.x-boss.position.x,dy=player.position.y-boss.position.y;const float absX=std::abs(dx);
    if(boss.attack==StoryBossAttack::None&&boss.attackTimer<=0){
        const int pick=GetRandomValue(0,boss.phase==1?3:5);
        boss.attack=pick==0?StoryBossAttack::ChainSwing:pick==1?StoryBossAttack::GroundSmash:pick==2?StoryBossAttack::Charge:pick==3?StoryBossAttack::PowerWave:StoryBossAttack::Frenzy;
        boss.attackElapsed=0;boss.attackTimer=boss.phase==3?1.05f:boss.phase==2?1.35f:1.7f;
    }
    if(boss.attack==StoryBossAttack::None&&absX>190)boss.position.x+=(dx>0?1:-1)*(boss.phase==3?130.f:boss.phase==2?100.f:78.f)*dt;
    if(boss.attack==StoryBossAttack::None&&std::abs(dy)>30)boss.position.y+=(dy>0?1:-1)*65.f*dt;

    const float tele=boss.attack==StoryBossAttack::Charge?.42f:boss.attack==StoryBossAttack::GroundSmash?.55f:boss.attack==StoryBossAttack::PowerWave?.48f:.30f;
    bool active=false;CombatBox hit{};
    if(boss.attack!=StoryBossAttack::None&&boss.attackElapsed>=tele){
        if(boss.attack==StoryBossAttack::ChainSwing){hit={boss.position.x-155,boss.position.y-115,310,110};active=boss.attackElapsed<=tele+.22f;}
        else if(boss.attack==StoryBossAttack::GroundSmash){hit={boss.position.x-290,boss.position.y-88,580,110};active=boss.attackElapsed<=tele+.20f;}
        else if(boss.attack==StoryBossAttack::Charge){boss.position.x+=(dx>0?1:-1)*560.f*dt;hit={boss.position.x-100,boss.position.y-110,200,120};active=boss.attackElapsed<=tele+.40f;}
        else if(boss.attack==StoryBossAttack::PowerWave){if(boss.attackElapsed<tele+.05f)SpawnBossPower();}
        else{hit={boss.position.x-190,boss.position.y-135,380,145};active=boss.attackElapsed<=tele+.30f;}
        if(active&&hit.Intersects(player.GetHurtbox())&&player.state!=PlayerState::Hit){int dmg=boss.phase==3?34:boss.phase==2?27:22;int before=player.hp;player.TakeDamage(dmg);damageTaken+=before-player.hp;combo=0;comboTimer=0;SpawnImpact(player.position,{255,70,50,255},true);hitstop=.10f;shake=.20f;}
    }
    if(boss.attack!=StoryBossAttack::None&&boss.attackElapsed>(boss.attack==StoryBossAttack::Charge?.92f:boss.attack==StoryBossAttack::PowerWave?1.0f:.86f)){boss.attack=StoryBossAttack::None;boss.attackElapsed=0;}
    boss.position.x=std::clamp(boss.position.x,5050.f,5850.f);boss.position.y=std::clamp(boss.position.y,kLaneMin,kLaneMax);
    HandleBossHits();
    if(boss.hp<=0)DefeatFinalBoss();
}

void Stage1StoryGame::DefeatFinalBoss(){
    if(boss.defeated)return;boss.defeated=true;boss.blocking=false;boss.attack=StoryBossAttack::None;score+=6000;xp+=650;coins+=1200;gems+=8;level=1+xp/1000;stageComplete=true;stageTime=std::max(stageTime,.1f);bestScore=std::max(bestScore,CalculateScore());bestRank=std::max(bestRank,CalculateRank());SaveProgress();SpawnImpact(boss.position,{255,155,45,255},true);shake=.5f;flow=StoryFlow::StageClear;arenaLocked=false;
}

void Stage1StoryGame::AdvanceScenario();

void Stage1StoryGame::UpdateParticles(float dt){for(auto&p:particles){p.life-=dt;p.position.x+=p.velocity.x*dt;p.position.y+=p.velocity.y*dt;p.velocity.x*=.92f;p.velocity.y*=.92f;}particles.erase(std::remove_if(particles.begin(),particles.end(),[](const StoryParticle&p){return p.life<=0;}),particles.end());}

int Stage1StoryGame::CalculateRank() const {float v=0;v+=std::max(0.f,360.f-stageTime)*.32f;v+=maxCombo*11.f;v+=player.hp*1.7f;v-=damageTaken*1.7f;v+=scenario==4&&stageComplete?100.f:0.f;if(v>=520)return 7;if(v>=430)return 6;if(v>=350)return 5;if(v>=275)return 4;if(v>=210)return 3;if(v>=145)return 2;if(v>=80)return 1;return 0;}
int Stage1StoryGame::CalculateScore() const {return score+player.hp*5+maxCombo*110;}
const char* Stage1StoryGame::RankText() const {static const char*r[] = {"D","C","B","A","S","SS","SSS","SSS"};return r[CalculateRank()];}

void Stage1StoryGame::LoadSave(){std::ifstream in(savePath);if(!in)return;in>>xp>>coins>>gems>>level>>bestScore>>bestRank;int d=1;in>>d;difficulty=d==0?StoryDifficulty::Easy:d==2?StoryDifficulty::Hard:StoryDifficulty::Normal;saveLoaded=true;}
void Stage1StoryGame::SaveProgress(){std::ofstream out(savePath,std::ios::trunc);if(!out)return;out<<xp<<' '<<coins<<' '<<gems<<' '<<level<<' '<<bestScore<<' '<<bestRank<<' '<<(difficulty==StoryDifficulty::Easy?0:difficulty==StoryDifficulty::Hard?2:1)<<'\n';}

void Stage1StoryGame::Update(float dt){
    dt=std::min(dt,.033f);
    if(flow==StoryFlow::Menu){if(IsKeyPressed(KEY_UP)||IsKeyPressed(KEY_DOWN)){difficulty=difficulty==StoryDifficulty::Normal?StoryDifficulty::Hard:difficulty==StoryDifficulty::Hard?StoryDifficulty::Easy:StoryDifficulty::Normal;SaveProgress();}if(IsKeyPressed(KEY_C)){flow=StoryFlow::Controls;return;}if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_J)){ResetRun();flow=StoryFlow::Intro;bannerTimer=2.4f;}if(IsKeyPressed(KEY_R)){LoadSave();}return;}
    if(flow==StoryFlow::Controls){if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_C))flow=StoryFlow::Menu;return;}
    if(IsKeyPressed(KEY_ESCAPE)){if(flow==StoryFlow::Combat||flow==StoryFlow::Boss)flow=StoryFlow::Pause;else if(flow==StoryFlow::Pause)flow=finalBossSpawned?StoryFlow::Boss:StoryFlow::Combat;}
    if(flow==StoryFlow::Pause)return;
    if(flow==StoryFlow::GameOver){if(IsKeyPressed(KEY_R)){ResetRun();flow=StoryFlow::Intro;}if(IsKeyPressed(KEY_Q))flow=StoryFlow::Menu;return;}
    if(flow==StoryFlow::StageClear){if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_R))flow=StoryFlow::Menu;return;}
    if(flow==StoryFlow::ScenarioClear){transitionTimer-=dt;storyTimer-=dt;if(transitionTimer<=0){BuildScenario(scenario);player.position.x=ScenarioStartX()+90;flow=StoryFlow::Combat;bannerTimer=2.0f;}return;}
    if(hitstop>0){hitstop-=dt;return;}
    shake=std::max(0.f,shake-dt);bannerTimer=std::max(0.f,bannerTimer-dt);UpdateParticles(dt);
    if(flow==StoryFlow::Intro){bannerTimer-=dt;if(bannerTimer<=0){flow=StoryFlow::Combat;SpawnWave(1);}return;}
    if(flow==StoryFlow::SubBossIntro){bannerTimer-=dt;if(bannerTimer<=0)flow=StoryFlow::Combat;UpdateCombat(dt);return;}
    if(flow==StoryFlow::Combat){stageTime+=dt;UpdateCombat(dt);}
    if(flow==StoryFlow::BossIntro||flow==StoryFlow::Boss){stageTime+=dt;UpdateBoss(dt);if(flow==StoryFlow::Boss&&player.state==PlayerState::Defeat)flow=StoryFlow::GameOver;}
    const float target=std::clamp(player.position.x,640.f,kStageEnd-640.f);cameraX+=(target-cameraX)*(1.f-std::pow(.001f,dt));
}

void Stage1StoryGame::DrawScenarioArt() const{
    const int s=scenario;DrawRectangle((int)(cameraX-700),0,1400,720,{11,16,19,255});
    if(s==1){for(int x=(int)(cameraX-700);x<cameraX+700;x+=170){DrawRectangle(x,285,90,250,{25,29,32,255});DrawRectangle(x+20,330,50,8,{150,50,55,160});DrawRectangle(x+8,385,74,5,{70,80,82,180});}}
    else if(s==2){for(int x=(int)(cameraX-700);x<cameraX+700;x+=210){DrawRectangle(x,260,145,275,{34,31,29,255});DrawRectangle(x+12,310,38,42,{190,135,65,110});DrawRectangle(x+65,310,58,42,{65,110,120,120});DrawLine(x+145,260,x+145,520,{105,75,50,210});}DrawRectangle((int)(cameraX-700),515,1400,80,{30,55,55,170});}
    else if(s==3){for(int x=(int)(cameraX-700);x<cameraX+700;x+=240){DrawRectangle(x,235,20,350,{38,45,48,255});DrawRectangle(x-15,255,50,12,{110,125,130,170});DrawRectangle(x+35,285,110,9,{90,105,110,130});DrawLine(x+10,350,x+155,300,{85,95,100,150});}DrawRectangle((int)(cameraX-700),410,1400,20,{60,65,65,220});}
    else {for(int x=(int)(cameraX-700);x<cameraX+700;x+=190){DrawRectangle(x,210,18,370,{33,38,39,255});DrawLine(x+9,230,x+130,380,{105,105,100,190});DrawLine(x+9,285,x+155,430,{105,105,100,150});DrawCircle(x+145,390,14,{110,105,95,200});}DrawRectangle((int)(cameraX-700),285,1400,12,{120,110,90,170});}
    Texture2D bg=AssetManager::Get().GetTexture("bg_industrial");if(bg.id)DrawTexturePro(bg,{0,0,(float)bg.width,(float)bg.height},{cameraX-640,0,1280,720},{0,0},0,WithAlpha(WHITE,s==1?.42f:s==2?.30f:s==3?.34f:.28f));
}

void Stage1StoryGame::DrawArenaLock() const{if(!arenaLocked)return;const float x=player.position.x+250;for(int i=0;i<8;++i){int xx=(int)x+i*24;DrawRectangle(xx,430,7,205,{85,220,120,90});DrawLine(xx,430,xx+45,385,{100,255,140,70});}const char*t="AREA LOCKED — DEFEAT THE GATEKEEPER";DrawText(t,640-MeasureText(t,18)/2,635,18,{130,245,170,220});}

void Stage1StoryGame::DrawBoss() const{
    if(!boss.position.x||boss.defeated)return;Vector2 p=boss.position.ToScreen();float sc=boss.phase==3?1.18f:boss.phase==2?1.08f:1.f;
    DrawEllipse((int)p.x,(int)p.y,82*sc,15*sc,{0,0,0,175});
    Color body=boss.phase==3?Color{100,45,40,255}:Color{55,55,58,255};DrawRectangle((int)(p.x-70*sc),(int)(p.y-125*sc),(int)(140*sc),(int)(105*sc),body);DrawCircle((int)p.x,(int)(p.y-150*sc),(int)(38*sc),{28,30,31,255});
    DrawRectangle((int)(p.x-32*sc),(int)(p.y-157*sc),(int)(64*sc),(int)(11*sc),{230,185,70,255});
    DrawLine((int)(p.x-52*sc),(int)(p.y-78*sc),(int)(p.x-125*sc),(int)(p.y-35*sc),{150,150,150,255});DrawLine((int)(p.x+52*sc),(int)(p.y-78*sc),(int)(p.x+125*sc),(int)(p.y-35*sc),{150,150,150,255});
    if(boss.blocking){DrawCircleLines((int)p.x,(int)(p.y-100*sc),92*sc+std::sin((float)GetTime()*12)*5,boss.phase==3?Color{255,70,45,170}:Color{90,200,255,180});DrawText("GUARD",(int)p.x-38,(int)p.y-220,18,{120,205,255,230});}
    if(boss.attack==StoryBossAttack::PowerWave)DrawCircleLines((int)p.x,(int)(p.y-80*sc),75+std::sin((float)GetTime()*10)*8,{255,75,45,180});
}

void Stage1StoryGame::DrawWorld() const{
    Camera2D c{};c.offset={640,360};c.target={cameraX,360};c.zoom=1;if(shake>0){c.target.x+=GetRandomValue(-100,100)*shake*7;c.target.y+=GetRandomValue(-100,100)*shake*4;}BeginMode2D(c);
    DrawScenarioArt();DrawRectangle(0,625,(int)kStageEnd,95,{9,12,14,255});for(int x=0;x<(int)kStageEnd;x+=150){DrawRectangle(x,617,110,10,{60,64,63,255});DrawRectangle(x+30,645,65,5,{90,75,50,210});}
    DrawArenaLock();
    std::vector<std::pair<float,int>> order{{player.position.y,-1}};for(size_t i=0;i<enemies.size();++i)if(enemies[i].active)order.push_back({enemies[i].position.y,(int)i});std::sort(order.begin(),order.end(),[](auto&a,auto&b){return a.first<b.first;});for(auto&o:order){if(o.second<0)player.Draw();else enemies[(size_t)o.second].Draw();}DrawBoss();
    for(const auto&p:projectiles){Vector2 s=p.position.ToScreen();DrawCircle((int)s.x,(int)s.y,20,WithAlpha(p.fromBoss?Color{255,65,45,255}:Color{40,220,255,255},.28f));DrawCircle((int)s.x,(int)s.y,11,p.fromBoss?Color{255,100,65,255}:Color{100,240,255,255});}
    for(const auto&p:particles){Vector2 s=p.position.ToScreen();float a=p.life/p.maxLife;DrawCircle((int)s.x,(int)s.y,p.size*a,WithAlpha(p.color,a));}EndMode2D();
}

void Stage1StoryGame::DrawHUD() const{
    DrawRectangle(14,12,535,118,{4,8,11,235});DrawText("RAYDEN CRUZ // DISTRICT FURY",27,20,20,{185,220,255,255});DrawText(TextFormat("HP %d/%d",player.hp,player.maxHp),27,49,15,WHITE);DrawRectangle(110,51,230,12,{25,25,30,255});DrawRectangle(110,51,(int)(230.f*player.hp/player.maxHp),12,{45,170,255,255});DrawText(TextFormat("SP %d   RAGE %d%%",player.sp,player.rage),27,72,15,player.isRageMode?Color{90,215,255,255}:WHITE);DrawText(TextFormat("SCENARIO %d/4",scenario),395,72,16,{255,205,80,255});if(combo>1)DrawText(TextFormat("COMBO x%d",combo),365,45,23,{255,190,65,255});
    DrawText(ScenarioName(),18,146,18,{210,225,230,235});DrawText(ScenarioObjective(),18,168,14,{170,190,195,220});DrawText(TextFormat("SCORE %d   TIME %5.1fs   DIFF %s",CalculateScore(),stageTime,DifficultyText()),18,190,14,WHITE);
    if(flow==StoryFlow::Boss||flow==StoryFlow::BossIntro){DrawRectangle(285,18,710,56,{5,7,9,235});DrawText("BRAKK // THE CHAIN",455,21,24,{255,205,95,255});DrawRectangle(350,53,580,13,{30,25,25,255});DrawRectangle(350,53,(int)(580.f*boss.hp/boss.maxHp),13,{220,65,55,255});DrawText(TextFormat("PHASE %d",boss.phase),945,51,15,WHITE);}
    if(bannerTimer>0){const char*t=ScenarioName();DrawText(t,640-MeasureText(t,30)/2,225,30,{210,235,240,235});DrawText(storyMessage.c_str(),640-MeasureText(storyMessage.c_str(),16)/2,263,16,{175,200,205,225});}
}

void Stage1StoryGame::DrawMenu() const{
    DrawRectangle(0,0,1280,720,{5,8,11,255});DrawText("DISTRICT FURY",370,125,64,{215,225,230,255});DrawText("A STORY BEAT-EM-UP",438,195,20,{110,205,220,255});DrawText("STAGE 1 // THE CHAIN ROUTE",390,255,25,{255,190,75,255});DrawText("ENTER — START STORY",455,340,23,WHITE);DrawText("C — CONTROLS",500,380,20,WHITE);DrawText(TextFormat("UP/DOWN — DIFFICULTY: %s",DifficultyText()),425,420,20,{185,205,215,255});DrawText(TextFormat("BEST SCORE: %d   BEST RANK: %s",bestScore,bestRank>=7?"SSS":bestRank==6?"SS":bestRank==5?"S":bestRank==4?"A":bestRank==3?"B":bestRank==2?"C":"D"),390,475,17,{150,170,175,255});DrawText("Rayden follows the chain symbols from the slums to Brakk's yard.",270,535,16,{160,180,185,220});DrawText("60 FPS  //  PC STORY MODE",500,620,14,{100,125,130,210});}
void Stage1StoryGame::DrawControls() const{DrawRectangle(0,0,1280,720,{5,8,11,250});DrawText("CONTROLS",515,100,48,WHITE);DrawText("W A S D",430,190,24,{185,220,240,255});DrawText("Move / change lane",620,190,20,WHITE);DrawText("J",430,235,24,{255,205,80,255});DrawText("Punch / combo",620,235,20,WHITE);DrawText("K",430,280,24,{255,205,80,255});DrawText("Kick / heavy hit",620,280,20,WHITE);DrawText("L",430,325,24,{80,220,255,255});DrawText("Energy Wave",620,325,20,WHITE);DrawText("SHIFT",430,370,24,{120,200,255,255});DrawText("Dash / brief invulnerability",620,370,20,WHITE);DrawText("SPACE",430,415,24,{100,220,255,255});DrawText("Rage Mode",620,415,20,WHITE);DrawText("ESC",430,460,24,WHITE);DrawText("Pause",620,460,20,WHITE);DrawText("C / ESC — BACK",500,585,18,{180,195,200,220});}
void Stage1StoryGame::DrawPause() const{DrawRectangle(0,0,1280,720,{0,0,0,170});DrawText("PAUSED",535,235,54,WHITE);DrawText("ESC — RESUME",500,330,22,WHITE);DrawText("R — RESTART",505,370,22,WHITE);DrawText("Q — QUIT TO MENU",470,410,22,WHITE);}
void Stage1StoryGame::DrawGameOver() const{DrawRectangle(0,0,1280,720,{0,0,0,205});DrawText("GAME OVER",430,230,62,{235,60,65,255});DrawText("The route is lost. Try again and keep the combo alive.",300,305,18,WHITE);DrawText("R — RESTART     Q — MENU",450,390,20,WHITE);}
void Stage1StoryGame::DrawScenarioClear() const{DrawRectangle(0,0,1280,720,{4,8,10,235});DrawText("ROUTE SECURED",430,160,52,{90,220,150,255});DrawText(TextFormat("SCENARIO %d CLEARED",scenario-1),465,235,28,WHITE);DrawText(storyMessage.c_str(),230,310,18,{175,200,205,235});DrawText("Next route unlocked. The story continues.",390,380,19,{255,200,80,255});}
void Stage1StoryGame::DrawStageClear() const{DrawRectangle(0,0,1280,720,{4,7,10,245});DrawText("STAGE 1 CLEAR",390,95,62,{90,225,155,255});DrawText("THE CHAIN ROUTE",440,165,26,{255,200,80,255});DrawText(TextFormat("TIME        %6.1fs",stageTime),400,245,22,WHITE);DrawText(TextFormat("ENEMIES     %6d",defeated),400,285,22,WHITE);DrawText(TextFormat("DAMAGE      %6d",damageTaken),400,325,22,WHITE);DrawText(TextFormat("MAX COMBO   %6d",maxCombo),400,365,22,WHITE);DrawText(TextFormat("SCORE       %6d",CalculateScore()),400,405,22,WHITE);DrawText(TextFormat("RANK        %s",RankText()),400,455,38,{255,205,70,255});DrawText(TextFormat("XP %d   COINS %d   GEMS %d",xp,coins,gems),400,515,18,{175,200,205,255});DrawText("ENTER — RETURN TO MENU",455,625,18,WHITE);}
void Stage1StoryGame::Draw() const{
    if(flow==StoryFlow::Menu){DrawMenu();return;}if(flow==StoryFlow::Controls){DrawControls();return;}
    DrawWorld();if(flow!=StoryFlow::StageClear)DrawHUD();if(flow==StoryFlow::Intro){DrawRectangle(0,0,1280,720,{4,8,11,130});DrawText("THE CHAIN ROUTE",420,280,48,{220,230,235,255});DrawText("ENTER / J — BEGIN",485,350,22,WHITE);}if(flow==StoryFlow::Pause)DrawPause();if(flow==StoryFlow::GameOver)DrawGameOver();if(flow==StoryFlow::ScenarioClear)DrawScenarioClear();if(flow==StoryFlow::StageClear)DrawStageClear();
}

}
