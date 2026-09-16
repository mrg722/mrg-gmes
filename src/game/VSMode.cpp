#include "game/VSMode.h"
#include "rendering/AssetManager.h"
#include "raylib.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace district_fury {
namespace {
constexpr float kMinX=180.0f,kMaxX=1120.0f,kMinY=505.0f,kMaxY=625.0f;
constexpr int kFieldCount=7;
const char* kStageNames[]={"STAGE 1 // SLUM DISTRICT","STAGE 2 // OLD STEEL YARD","STAGE 3 // ASTRA TOWER"};
const int kScenarioCounts[]={4,5,3};
const char* kStage1Scenarios[]={"BARRIO BAJO // BLOQUE 17","MERCADO ANTIGUO // LINEA DEL CANAL","PUERTA DE ACERO // RUTA DE CARGA","ASTILLERO DE CADENAS // TERRITORIO DE BRAKK"};
const char* kStage2Scenarios[]={"DEEP LINE // ENTRADA","OLD STEEL YARD // FUNDICION","ZONA QUIMICA // PROCESAMIENTO","OLD STEEL YARD // DEPOSITOS","CAMARA DE GRINDER"};
const char* kStage3Scenarios[]={"PUBLIC ATRIUM","RESEARCH FLOOR","EXECUTIVE CORE"};
const char* kEnemyNames[]={"PUNK","BRUTE","CHARGER","ENFORCER","CHEMICAL SOLDIER","URBAN NINJA","MUTANT","ARMORED GUARD"};
StreetEnemyType NextEnemyType(StreetEnemyType t,int dir){int i=(static_cast<int>(t)+dir+8)%8;return static_cast<StreetEnemyType>(i);}
}
VSMode::VSMode()=default;
void VSMode::Init(){flow=VSFlow::Select;stage=0;scenario=0;enemyCount=1;cursor=0;exitRequested=false;playerDefeated=false;enemyTypes={StreetEnemyType::Punk,StreetEnemyType::Brute,StreetEnemyType::Charger,StreetEnemyType::Enforcer};enemies.clear();projectiles.clear();player.Reset();player.position={360,585,0};shake=0;}
bool VSMode::ShouldExit()const{return exitRequested;}
void VSMode::ClearExit(){exitRequested=false;}
int VSMode::ScenarioCount()const{return kScenarioCounts[std::clamp(stage,0,2)];}
const char* VSMode::StageName()const{return kStageNames[std::clamp(stage,0,2)];}
const char* VSMode::ScenarioText()const{if(stage==0)return kStage1Scenarios[std::clamp(scenario,0,3)];if(stage==1)return kStage2Scenarios[std::clamp(scenario,0,4)];return kStage3Scenarios[std::clamp(scenario,0,2)];}
const char* VSMode::BackgroundKey()const{if(stage==0&&scenario==1)return "bg_mercado_antiguo";if(stage==1&&scenario==2)return "bg_zona_quimica";return stage==0?"bg_industrial":"bg_steel_deep";}
const char* VSMode::EnemyTypeName(StreetEnemyType t){return kEnemyNames[static_cast<int>(t)];}
void VSMode::ResetFight(){
 player.Reset();player.position={360,585,0};player.rage=player.maxRage;player.sp=player.maxSp;playerDefeated=false;shake=0;enemies.clear();projectiles.clear();
 const std::array<float,4> xs{700,835,970,1105}; const std::array<float,4> ys{575,535,610,555};
 for(int i=0;i<enemyCount;++i){StreetEnemy e;e.Init({xs[(size_t)i],ys[(size_t)i],0},enemyTypes[(size_t)i]);e.active=true;enemies.push_back(e);}
}
void VSMode::StartFight(){flow=VSFlow::Fight;ResetFight();}
void VSMode::SpawnEnergyProjectile(){
 const float direction=player.facing==Facing::Right?1.0f:-1.0f;
 projectiles.push_back({{player.position.x+direction*68.0f,player.position.y-72.0f,0},direction*760.0f,0.9f,18.0f,player.isRageMode?30:22,true});
 shake=std::max(shake,.06f);
}
void VSMode::UpdateProjectiles(float dt){
 for(auto& projectile:projectiles){
  if(!projectile.active)continue;
  projectile.position.x+=projectile.velocity*dt;projectile.life-=dt;
  if(projectile.life<=0){projectile.active=false;continue;}
  const CombatBox box{projectile.position.x-projectile.radius,projectile.position.y-projectile.radius,projectile.radius*2,projectile.radius*2};
  for(auto& e:enemies){
   if(!e.active||e.IsDefeated()||!box.Intersects(e.GetHurtbox()))continue;
   e.TakeDamage(projectile.damage,{projectile.velocity>0?430.0f:-430.0f,0,0});projectile.active=false;shake=std::max(shake,.12f);break;
  }
 }
 projectiles.erase(std::remove_if(projectiles.begin(),projectiles.end(),[](const VSEnergyProjectile& p){return !p.active;}),projectiles.end());
}
void VSMode::Update(float dt){
 dt=std::min(dt,.033f);
 if(flow==VSFlow::Select){
  if(IsKeyPressed(KEY_ESCAPE)){exitRequested=true;return;}
  if(IsKeyPressed(KEY_UP))cursor=(cursor+kFieldCount-1)%kFieldCount;
  if(IsKeyPressed(KEY_DOWN))cursor=(cursor+1)%kFieldCount;
  if(IsKeyPressed(KEY_LEFT)||IsKeyPressed(KEY_RIGHT)){int dir=IsKeyPressed(KEY_RIGHT)?1:-1;if(cursor==0){stage=(stage+dir+3)%3;scenario=std::clamp(scenario,0,ScenarioCount()-1);}else if(cursor==1)scenario=std::clamp(scenario+dir,0,ScenarioCount()-1);else if(cursor==2)enemyCount=std::clamp(enemyCount+dir,1,4);else{int slot=cursor-3;enemyTypes[(size_t)slot]=NextEnemyType(enemyTypes[(size_t)slot],dir);}}
  if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_J))StartFight();return;
 }
 if(IsKeyPressed(KEY_ESCAPE)){flow=VSFlow::Select;enemies.clear();projectiles.clear();return;}
 if(IsKeyPressed(KEY_R)){ResetFight();return;}
 if(playerDefeated){if(IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_J))ResetFight();return;}
 player.Update(dt);
 if(player.state==PlayerState::Attack&&player.attackType==AttackType::Energy&&player.energyReleased){SpawnEnergyProjectile();player.energyReleased=false;}
 player.position.x=std::clamp(player.position.x,kMinX,kMaxX);player.position.y=std::clamp(player.position.y,kMinY,kMaxY);
 for(auto& e:enemies)if(e.active&&!e.IsDefeated()){e.Update(dt,player);e.position.x=std::clamp(e.position.x,kMinX,kMaxX);e.position.y=std::clamp(e.position.y,kMinY,kMaxY);}
 UpdateProjectiles(dt);
 if(player.AttackIsActive()&&!player.hasHit&&player.attackType!=AttackType::Energy){const CombatBox hit=player.GetAttackHitbox();for(auto& e:enemies)if(e.active&&!e.IsDefeated()&&hit.Intersects(e.GetHurtbox())){const float d=player.facing==Facing::Right?1.f:-1.f;e.TakeDamage(player.GetAttackDamage()+(player.isRageMode?5:0),{d*player.GetAttackKnockback(),0,0});player.hasHit=true;shake=.08f;break;}}
 if(player.state!=PlayerState::Defeat)for(auto& e:enemies)if(e.active&&!e.IsDefeated()&&!e.hasHit&&e.AttackIsActive()&&e.GetAttackHitbox().Intersects(player.GetHurtbox())){player.TakeDamage((int)e.attackDamage);e.hasHit=true;shake=.10f;break;}
 playerDefeated=player.state==PlayerState::Defeat;shake=std::max(0.0f,shake-dt);
}
void VSMode::DrawBackground()const{DrawRectangle(0,0,1280,720,{5,8,12,255});const Texture2D bg=AssetManager::Get().GetTexture(BackgroundKey());if(bg.id)DrawTexturePro(bg,{0,0,(float)bg.width,(float)bg.height},{0,0,1280,720},{0,0},0,WHITE);DrawRectangle(0,625,1280,95,{7,10,13,220});for(int x=0;x<1280;x+=160){DrawRectangle(x,617,108,8,{55,60,61,235});DrawRectangle(x+25,647,68,5,{94,78,48,190});}}
void VSMode::DrawHud()const{
 DrawRectangle(16,16,530,122,{3,7,11,232});const Texture2D portrait=AssetManager::Get().GetTexture("rayden_clean");if(portrait.id)DrawTexturePro(portrait,{0,0,96,96},{28,28,70,70},{0,0},0,WHITE);DrawRectangleLines(25,25,76,76,{70,200,235,160});
 DrawText("RAYDEN CRUZ",112,25,20,{220,235,240,255});DrawText("VIDA",112,53,12,WHITE);DrawRectangle(162,53,250,12,{24,27,31,240});DrawRectangle(162,53,(int)(250.f*player.hp/std::max(1,player.maxHp)),12,{225,65,65,255});DrawText("SP",112,76,12,{120,215,255,255});DrawRectangle(162,76,116,8,{22,26,31,240});DrawRectangle(162,76,(int)(116.f*player.sp/std::max(1,player.maxSp)),8,{55,190,245,255});DrawText("FURIA",294,76,12,{255,205,75,255});DrawRectangle(340,76,116,8,{28,25,21,240});DrawRectangle(340,76,(int)(116.f*player.rage/std::max(1,player.maxRage)),8,player.isRageMode?Color{255,75,60,255}:Color{255,190,60,255});DrawText(TextFormat("ESCUDO %d%%",(int)(100.f*player.shield/std::max(1,player.maxShield))),112,100,12,{105,215,255,235});DrawText(TextFormat("STAGE %d  //  %s",stage+1,ScenarioText()),294,100,11,{175,200,210,230});
 DrawRectangle(818,16,446,122,{3,7,11,220});DrawText(StageName(),840,25,18,{255,205,75,255});DrawText(TextFormat("ENEMIGOS %d/4",enemyCount),840,52,13,WHITE);for(int i=0;i<enemyCount;++i){const auto&e=enemies[(size_t)i];DrawText(TextFormat("%d  %s",i+1,EnemyTypeName(e.type)),840,72+i*15,11,e.IsDefeated()?Color{110,120,125,180}:WHITE);}
}
void VSMode::DrawSelection()const{DrawBackground();DrawRectangle(205,48,870,610,{3,7,11,242});DrawRectangleLines(205,48,870,610,{55,90,105,170});DrawRectangle(205,48,6,610,{60,205,240,230});DrawRectangle(1069,48,6,610,{255,205,75,210});DrawText("MODO VS // LABORATORIO",405,76,36,{225,235,240,255});DrawText("PRUEBA DIRECTA DE SPRITES, ESCENARIOS Y COMBATE",335,121,13,{120,185,205,240});const int y[]={165,214,263,312,361,410,459};const Color active={255,215,80,255};const char* labels[]={"STAGE","ESCENARIO","CANTIDAD","ENEMIGO 1","ENEMIGO 2","ENEMIGO 3","ENEMIGO 4"};for(int i=0;i<kFieldCount;++i){bool selected=cursor==i;DrawRectangle(335,y[i]-8,610,38,selected?Color{20,28,34,230}:Color{8,15,21,190});DrawRectangleLines(335,y[i]-8,610,38,selected?Color{255,205,75,210}:Color{70,95,105,90});DrawText(labels[i],360,y[i],16,selected?active:WHITE);}DrawText(StageName(),600,y[0],16,{190,220,230,255});DrawText(ScenarioText(),600,y[1],16,{190,220,230,255});DrawText(TextFormat("%d ENEMIGO%s",enemyCount,enemyCount==1?"":"S"),600,y[2],16,{190,220,230,255});for(int i=0;i<4;++i){bool enabled=i<enemyCount;DrawText(EnemyTypeName(enemyTypes[(size_t)i]),600,y[3+i],16,enabled?Color{190,220,230,255}:Color{85,95,100,130});}DrawText("↑/↓ CAMPO    ←/→ CAMBIAR    ENTER/J INICIAR",391,530,14,{170,195,205,245});DrawText("ESC VOLVER AL MENU",485,559,13,{130,155,165,220});}
void VSMode::DrawFight()const{
 DrawBackground();std::array<std::pair<float,int>,5> order{};int n=0;order[n++]={player.position.y,-1};for(int i=0;i<(int)enemies.size();++i)if(enemies[(size_t)i].active)order[n++]={enemies[(size_t)i].position.y,i};std::sort(order.begin(),order.begin()+n,[](auto&a,auto&b){return a.first<b.first;});for(int i=0;i<n;++i)if(order[i].second<0)player.Draw();else enemies[(size_t)order[i].second].Draw();
 for(const auto& projectile:projectiles){const Vector2 p=projectile.position.ToScreen();DrawCircle((int)p.x,(int)p.y,projectile.radius,{30,185,255,70});DrawCircle((int)p.x,(int)p.y,projectile.radius*.62f,{110,235,255,255});DrawCircleLines((int)p.x,(int)p.y,projectile.radius+4,{90,225,255,145});const float trail=projectile.velocity>0?-34.f:34.f;DrawLine((int)(p.x+trail),(int)p.y,(int)p.x,(int)p.y,{80,220,255,165});}
 for(int i=0;i<(int)enemies.size();++i){const auto&e=enemies[(size_t)i];Vector2 p=e.position.ToScreen();const int w=96,x=(int)p.x-w/2,y=(int)p.y-125;DrawRectangle(x,y,w,7,{18,20,23,220});if(!e.IsDefeated())DrawRectangle(x+1,y+1,(int)((w-2)*e.hp/std::max(1.f,(float)e.maxHp)),5,{220,65,65,255});DrawText(TextFormat("%d %s",i+1,EnemyTypeName(e.type)),x,y-16,10,e.IsDefeated()?Color{120,130,135,170}:WHITE);}
 DrawHud();DrawRectangle(260,657,760,45,{3,7,11,225});DrawText("J GOLPE   K PATADA   L ENERGIA   B BLOQUEO   SHIFT DASH   SPACE FURIA",296,669,12,{170,200,210,240});DrawText("R REINICIAR   ESC CONFIGURACION",489,687,11,{135,155,165,220});if(playerDefeated){DrawRectangle(0,0,1280,720,{0,0,0,160});DrawText("RAYDEN DERROTADO",438,288,46,{240,80,80,255});DrawText("ENTER/J REINICIAR   ESC CONFIGURACION",430,357,18,WHITE);}}
void VSMode::Draw()const{if(flow==VSFlow::Select)DrawSelection();else DrawFight();}
}
