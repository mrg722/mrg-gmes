#include "game/StreetEnemy.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {
struct Stats { int hp; float speed; float damage; float range; float depth; float attackDuration; float scale; Color fallbackTint; float bodyWidth; float bodyHeight; };
Stats GetStats(StreetEnemyType type) {
    switch(type) {
        case StreetEnemyType::Brute: return {130,95.0f,20.0f,132.0f,46.0f,0.72f,1.10f,{220,140,70,255},64.0f,112.0f};
        case StreetEnemyType::Charger: return {72,220.0f,14.0f,120.0f,42.0f,0.54f,1.00f,{90,155,220,255},54.0f,108.0f};
        case StreetEnemyType::Enforcer: return {190,122.0f,24.0f,146.0f,50.0f,0.76f,1.12f,{235,185,70,255},66.0f,116.0f};
        case StreetEnemyType::ChemicalSoldier: return {155,118.0f,22.0f,154.0f,58.0f,0.82f,1.08f,{65,205,105,255},64.0f,116.0f};
        case StreetEnemyType::UrbanNinja: return {82,245.0f,18.0f,138.0f,48.0f,0.48f,1.00f,{80,205,220,255},52.0f,108.0f};
        case StreetEnemyType::Mutant: return {220,82.0f,29.0f,142.0f,54.0f,0.86f,1.16f,{105,205,75,255},70.0f,122.0f};
        case StreetEnemyType::ArmoredGuard: return {250,105.0f,27.0f,150.0f,54.0f,0.80f,1.14f,{105,145,180,255},68.0f,118.0f};
        default: return {58,172.0f,12.0f,110.0f,38.0f,0.62f,1.00f,{225,65,80,255},54.0f,108.0f};
    }
}
Color ThreatColor(StreetEnemyType type) { switch(type){case StreetEnemyType::Brute:return {220,140,70,255};case StreetEnemyType::Charger:return {90,155,220,255};case StreetEnemyType::Enforcer:return {235,185,70,255};case StreetEnemyType::ChemicalSoldier:return {70,220,115,255};case StreetEnemyType::UrbanNinja:return {90,210,220,255};case StreetEnemyType::Mutant:return {120,225,75,255};case StreetEnemyType::ArmoredGuard:return {120,165,210,255};default:return {225,65,80,255};} }
float DepthScale(float y){const float t=std::clamp((y-kLaneMinY)/(kLaneMaxY-kLaneMinY),0.0f,1.0f);return 0.86f+0.26f*t;}
const char* TextureKeyFor(StreetEnemyType type){switch(type){case StreetEnemyType::Brute:return "brute_clean";case StreetEnemyType::Charger:return "charger_clean";case StreetEnemyType::Enforcer:return "enforcer_clean";case StreetEnemyType::ChemicalSoldier:return "enforcer_clean";case StreetEnemyType::UrbanNinja:return "charger_clean";case StreetEnemyType::Mutant:return "brute_clean";case StreetEnemyType::ArmoredGuard:return "enforcer_clean";default:return "punk_clean";}}
struct EnemyAnimationLayout { int columns; int rows; int idleStart; int idleEnd; int walkStart; int walkEnd; int attackStart; int attackEnd; int hitFrame; int deathStart; int deathEnd; };
EnemyAnimationLayout LayoutFor(StreetEnemyType type){switch(type){case StreetEnemyType::Charger:return {4,3,0,3,4,7,8,9,8,10,11};case StreetEnemyType::Brute:return {4,3,0,3,4,7,8,9,8,10,11};case StreetEnemyType::Enforcer:return {4,3,0,3,4,7,8,9,8,10,11};case StreetEnemyType::ChemicalSoldier:return {4,3,0,3,4,7,8,9,8,10,11};case StreetEnemyType::UrbanNinja:return {4,3,0,3,4,7,8,9,8,10,11};case StreetEnemyType::Mutant:return {4,3,0,3,4,7,8,9,8,10,11};case StreetEnemyType::ArmoredGuard:return {4,3,0,3,4,7,8,9,8,10,11};default:return {4,3,0,3,4,7,8,9,8,10,11};}}
void EnsureAnimator(Animator& a,StreetEnemyType type){if(a.texture.id!=0)return;const EnemyAnimationLayout layout=LayoutFor(type);Texture2D t=AssetManager::Get().GetTexture(TextureKeyFor(type));if(t.id!=0){a.Init(t,layout.columns,layout.rows,true);a.Play({layout.idleStart,layout.idleEnd,0.12f,true});}}
void PlayIdle(Animator& animator,StreetEnemyType type){const EnemyAnimationLayout layout=LayoutFor(type);if(animator.isFinished||!animator.isPlaying||animator.currentFrame<layout.idleStart||animator.currentFrame>layout.idleEnd)animator.Play({layout.idleStart,layout.idleEnd,.12f,true});}
void PlayWalk(Animator& animator,StreetEnemyType type){const EnemyAnimationLayout layout=LayoutFor(type);if(animator.isFinished||!animator.isPlaying||animator.currentFrame<layout.walkStart||animator.currentFrame>layout.walkEnd)animator.Play({layout.walkStart,layout.walkEnd,.10f,true});}
void DrawFallbackEnemy(Vector2 p,const Stats&s,StreetEnemyType type,float scale,Color tint){const float w=s.bodyWidth*scale,h=s.bodyHeight*scale;const int x=(int)(p.x-w*.5f),y=(int)(p.y-h),bw=(int)w,bh=(int)(h*.62f),by=y+(int)(h*.32f);if(type==StreetEnemyType::Punk){DrawTriangle({(float)(x+bw/2),(float)y},{(float)(x+bw),(float)by},{(float)x,(float)by},tint);DrawRectangle(x+bw/4,by,bw/2,bh,tint);}else if(type==StreetEnemyType::Charger||type==StreetEnemyType::UrbanNinja){DrawRectangle(x+bw/5,by,bw*3/5,bh,tint);DrawLine(x+bw/5,by+bh,x,(int)p.y,tint);DrawLine(x+bw*4/5,by+bh,x+bw,(int)p.y,tint);}else if(type==StreetEnemyType::Brute||type==StreetEnemyType::Mutant){DrawRectangle(x,by,bw,bh,tint);DrawCircle(x+bw/2,y+(int)(h*.2f),bw*.24f,tint);if(type==StreetEnemyType::Mutant){DrawCircle(x+bw/2-13,y+(int)(h*.2f),4,{180,255,80,255});DrawCircle(x+bw/2+13,y+(int)(h*.2f),4,{180,255,80,255});}}else{DrawRectangle(x+bw/8,by,bw*3/4,bh,tint);DrawRectangle(x+bw/4,y,bw/2,(int)(h*.32f),tint);DrawRectangle(x,by+bh/4,bw/6,bh/2,tint);DrawRectangle(x+bw*5/6,by+bh/4,bw/6,bh/2,tint);}}
}
StreetEnemy::StreetEnemy(){Init({900.0f,560.0f,0.0f},StreetEnemyType::Punk);}
void StreetEnemy::Init(Vector3D startPos,StreetEnemyType enemyType){const Stats s=GetStats(enemyType);position=startPos;velocity={0,0,0};facing=Facing::Left;state=StreetEnemyState::Idle;type=enemyType;active=false;hp=s.hp;maxHp=s.hp;moveSpeed=s.speed;attackDamage=s.damage;attackRange=s.range;attackDepth=s.depth;attackDuration=s.attackDuration;stateTimer=0;attackElapsed=0;attackCooldown=0;hasHit=false;animator=Animator{};}
void StreetEnemy::Activate(){active=true;if(state==StreetEnemyState::Defeat)return;state=StreetEnemyState::Idle;hasHit=false;}
void StreetEnemy::Update(float dt,const Player& player){
    if(!active)return; EnsureAnimator(animator,type); animator.Update(dt); attackCooldown=std::max(0.0f,attackCooldown-dt);
    if(state==StreetEnemyState::Defeat){stateTimer-=dt;if(stateTimer<=0)active=false;return;}
    if(state==StreetEnemyState::Hit){stateTimer-=dt;position.x+=velocity.x*dt;position.y+=velocity.y*dt;velocity.x*=.84f;velocity.y*=.84f;if(stateTimer<=0){state=StreetEnemyState::Idle;PlayIdle(animator,type);}position.x=std::clamp(position.x,kStageStartX,kStageEndX-90.f);position.y=std::clamp(position.y,kLaneMinY,kLaneMaxY);return;}
    if(state==StreetEnemyState::Attack){stateTimer-=dt;attackElapsed+=dt;if(stateTimer<=0||animator.isFinished){state=StreetEnemyState::Idle;attackCooldown=.65f;const EnemyAnimationLayout layout=LayoutFor(type);animator.Play({layout.idleStart,layout.idleEnd,.12f,true});}return;}
    if(player.state==PlayerState::Defeat)return;
    const float dx=player.position.x-position.x,dy=player.position.y-position.y;const float horizontal=std::abs(dx),depth=std::abs(dy),distance=std::sqrt(dx*dx+dy*dy);facing=dx>=0?Facing::Right:Facing::Left;
    const float stopDistance=std::max(82.0f,attackRange*0.62f);
    if(horizontal<attackRange&&depth<attackDepth&&horizontal>stopDistance&&attackCooldown<=0){state=StreetEnemyState::Attack;stateTimer=attackDuration;attackElapsed=0;hasHit=false;const float speed=type==StreetEnemyType::UrbanNinja?.055f:type==StreetEnemyType::ChemicalSoldier?.09f:.10f;const EnemyAnimationLayout layout=LayoutFor(type);animator.Play({layout.attackStart,layout.attackEnd,speed,false});return;}
    if(horizontal<=stopDistance&&depth<attackDepth){state=StreetEnemyState::Idle;PlayIdle(animator,type);position.y+=(dy>0?1.f:-1.f)*std::min(std::abs(dy),18.f)*dt;return;}
    if(distance<820.f){state=StreetEnemyState::Chase;float chaseSpeed=moveSpeed;if(type==StreetEnemyType::Mutant&&distance>280)chaseSpeed*=.82f;if(type==StreetEnemyType::ChemicalSoldier&&horizontal<360)chaseSpeed*=.78f;if(distance>.001f){position.x+=(dx/distance)*chaseSpeed*dt;position.y+=(dy/distance)*chaseSpeed*.72f*dt;}PlayWalk(animator,type);}
    else{state=StreetEnemyState::Idle;PlayIdle(animator,type);}
    position.x=std::clamp(position.x,kStageStartX,kStageEndX-90.f);position.y=std::clamp(position.y,kLaneMinY,kLaneMaxY);
}
bool StreetEnemy::AttackIsActive() const{return state==StreetEnemyState::Attack&&attackElapsed>=attackDuration*.34f&&attackElapsed<=attackDuration*.74f;}
bool StreetEnemy::IsDefeated() const{return state==StreetEnemyState::Defeat;}
CombatBox StreetEnemy::GetHurtbox() const{if(!active||IsDefeated())return{};const Stats s=GetStats(type);return{position.x-s.bodyWidth*.5f,position.y-s.bodyHeight,s.bodyWidth,s.bodyHeight};}
CombatBox StreetEnemy::GetAttackHitbox() const{if(!AttackIsActive())return{};const float d=facing==Facing::Right?1.f:-1.f;const float w=attackRange*.68f,h=52.f,cx=position.x+d*attackRange*.52f,cy=position.y-66;return{cx-w*.5f,cy-h*.5f,w,h};}
const char* StreetEnemy::GetTypeName() const{switch(type){case StreetEnemyType::Brute:return "BRUTE";case StreetEnemyType::Charger:return "CHARGER";case StreetEnemyType::Enforcer:return "ENFORCER";case StreetEnemyType::ChemicalSoldier:return "CHEMICAL";case StreetEnemyType::UrbanNinja:return "URBAN NINJA";case StreetEnemyType::Mutant:return "MUTANT";case StreetEnemyType::ArmoredGuard:return "ARMORED";default:return "PUNK";}}
void StreetEnemy::TakeDamage(int damage,Vector3D knockback){if(!active||state==StreetEnemyState::Defeat)return;hp=std::max(0,hp-damage);velocity=knockback;const EnemyAnimationLayout layout=LayoutFor(type);if(hp==0){state=StreetEnemyState::Defeat;stateTimer=.85f;if(animator.texture.id!=0)animator.Play({layout.deathStart,layout.deathEnd,.14f,false});else{animator.isFinished=true;animator.isPlaying=false;}}else{state=StreetEnemyState::Hit;stateTimer=.30f;if(animator.texture.id!=0)animator.Play({layout.hitFrame,layout.hitFrame,.09f,false});}}
void StreetEnemy::Draw() const{if(!active)return;const Stats s=GetStats(type);const Vector2 p=position.ToScreen();const float ds=DepthScale(position.y),vs=(animator.normalizedAtlas?s.scale:.86f)*ds;DrawEllipse((int)p.x,(int)p.y,26*s.scale*ds,8.5f*ds,{0,0,0,145});if(animator.texture.id!=0){Color tint=animator.normalizedAtlas?WHITE:s.fallbackTint;if(state==StreetEnemyState::Hit)tint={255,215,215,255};if(state==StreetEnemyState::Defeat)tint={175,175,175,255};animator.Draw(p,vs,facing==Facing::Left,tint);}else{Color tint=state==StreetEnemyState::Hit?WHITE:state==StreetEnemyState::Defeat?Color{110,110,115,255}:s.fallbackTint;DrawFallbackEnemy(p,s,type,ds,tint);}if(state!=StreetEnemyState::Defeat){const int width=s.scale>=1.08f?82:68;const int x=(int)(p.x-width*.5f),y=(int)(p.y-s.bodyHeight*ds-11);DrawRectangle(x,y,width,6,{8,9,11,210});DrawRectangle(x,y,(int)(width*((float)hp/maxHp)),6,ThreatColor(type));}}
}
