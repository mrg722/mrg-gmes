#include "game/StreetEnemy.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

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

const char* TextureKeyFor(StreetEnemyType type){
    switch(type){
        case StreetEnemyType::Brute:return "brute_clean";
        case StreetEnemyType::Charger:return "charger_clean";
        case StreetEnemyType::Enforcer:return "enforcer_clean";
        case StreetEnemyType::ChemicalSoldier:return "chemical_soldier_clean";
        case StreetEnemyType::UrbanNinja:return "urban_ninja_clean";
        case StreetEnemyType::Mutant:return "mutant_clean";
        case StreetEnemyType::ArmoredGuard:return "armored_guard_clean";
        default:return "punk_clean";
    }
}

struct EnemyAnimationLayout { int columns; int rows; int idleStart; int idleEnd; int walkStart; int walkEnd; int attackStart; int attackEnd; int hitFrame; int deathStart; int deathEnd; };
EnemyAnimationLayout LayoutFor(StreetEnemyType){ return {4,3,0,3,0,3,4,7,8,10,11}; }

// Coordenadas de cada recorte dentro de su celda 128x128.
// pivotX/pivotY se expresan en coordenadas de la celda original, antes del recorte.
// Cada enemigo tiene su propio conjunto: no se reutilizan bounds de otro atlas.
struct FrameBounds { int x; int y; int w; int h; float pivotX; float pivotY; };
using FrameSet = std::array<FrameBounds,12>;

const FrameSet kPunkFrames = {{
    {22,6,81,121,63.3f,127.0f},{15,8,81,119,55.7f,127.0f},{19,6,84,121,60.8f,127.0f},{25,5,84,122,66.9f,127.0f},
    {17,11,89,112,61.3f,123.0f},{12,11,116,112,60.7f,123.0f},{0,9,107,114,59.6f,123.0f},{21,10,89,113,66.0f,123.0f},
    {17,3,90,107,55.6f,110.0f},{13,35,115,74,59.6f,109.0f},{0,36,128,73,54.2f,109.0f},{0,70,123,40,57.7f,110.0f}
}};
const FrameSet kChargerFrames = {{
    {19,10,91,118,63.7f,128.0f},{20,11,90,117,63.9f,128.0f},{18,10,89,118,60.6f,128.0f},{14,10,93,118,59.8f,128.0f},
    {10,0,118,128,60.5f,128.0f},{0,0,116,128,33.8f,128.0f},{6,0,122,128,55.8f,128.0f},{0,0,112,128,65.0f,128.0f},
    {10,0,99,117,67.7f,117.0f},{19,0,109,116,66.2f,116.0f},{0,0,128,117,60.3f,117.0f},{0,0,117,115,54.7f,115.0f}
}};
const FrameSet kBruteFrames = {{
    {26,7,88,121,68.4f,128.0f},{19,10,99,118,69.3f,128.0f},{17,10,96,118,64.1f,128.0f},{16,9,87,119,59.4f,128.0f},
    {16,0,103,128,65.8f,128.0f},{11,0,117,128,62.7f,128.0f},{0,0,128,128,60.8f,128.0f},{0,0,112,128,61.5f,128.0f},
    {26,8,96,105,79.0f,113.0f},{24,16,91,97,61.2f,113.0f},{23,42,105,73,66.3f,115.0f},{0,68,121,48,55.7f,116.0f}
}};

// Metadatos calibrados para las cuatro hojas nuevas suministradas en esta tarea.
const FrameSet kChemicalSoldierFrames = {{
    {26,11,82,117,67,128},{13,16,89,112,64,128},{11,17,87,111,61,128},{6,13,93,115,58,128},
    {20,0,108,128,63,128},{0,0,128,128,66,128},{0,0,128,128,61,128},{0,0,116,119,54,92},
    {20,0,90,114,70,114},{8,0,120,114,52,114},{0,0,128,106,52,106},{0,61,107,46,54,107}
}};
const FrameSet kUrbanNinjaFrames = {{
    {26,18,87,110,64,128},{16,18,100,110,65,128},{10,20,97,108,61,128},{9,20,95,108,60,128},
    {21,22,107,106,69,128},{0,0,128,124,82,124},{0,0,128,126,35,121},{0,0,117,126,61,126},
    {22,0,93,113,65,113},{25,31,103,81,60,112},{0,42,128,67,52,109},{0,66,104,41,52,107}
}};
const FrameSet kMutantFrames = {{
    {19,13,108,115,64,128},{8,15,120,113,62,128},{0,13,114,115,59,128},{0,13,110,115,57,128},
    {14,0,114,128,63,128},{0,0,128,123,64,123},{0,0,128,128,61,128},{0,0,109,128,55,128},
    {16,0,112,116,58,116},{19,8,100,108,62,116},{0,0,128,114,54,114},{0,0,113,113,53,113}
}};
const FrameSet kArmoredGuardFrames = {{
    {20,11,93,117,64,128},{16,12,94,116,64,128},{8,13,120,115,63,128},{0,14,109,114,62,128},
    {14,0,111,128,65,128},{1,0,127,128,66,128},{0,0,128,128,64,128},{0,0,115,128,63,128},
    {18,0,99,113,60,113},{19,0,109,112,60,112},{0,0,113,112,55,112},{0,0,116,109,55,109}
}};

const FrameSet& BoundsFor(StreetEnemyType type){
    switch(type){
        case StreetEnemyType::Punk:return kPunkFrames;
        case StreetEnemyType::Charger:return kChargerFrames;
        case StreetEnemyType::Brute:return kBruteFrames;
        case StreetEnemyType::Enforcer:return kPunkFrames;
        case StreetEnemyType::ChemicalSoldier:return kChemicalSoldierFrames;
        case StreetEnemyType::UrbanNinja:return kUrbanNinjaFrames;
        case StreetEnemyType::Mutant:return kMutantFrames;
        case StreetEnemyType::ArmoredGuard:return kArmoredGuardFrames;
        default:return kPunkFrames;
    }
}

std::vector<SpriteFrame> BuildSpriteFrames(StreetEnemyType type){
    const FrameSet& bounds=BoundsFor(type);
    std::vector<SpriteFrame> result;
    result.reserve(bounds.size());
    for(std::size_t i=0;i<bounds.size();++i){
        const FrameBounds& b=bounds[i];
        SpriteFrame frame;
        frame.source={(float)((i%4)*128+b.x),(float)((i/4)*128+b.y),(float)b.w,(float)b.h};
        frame.width=(float)b.w;
        frame.height=(float)b.h;
        // Convertimos el pivot de la celda al origen del recorte. Animator::Draw
        // vuelve a reflejarlo cuando el sprite se dibuja mirando hacia la izquierda.
        frame.pivotX=b.pivotX-(float)b.x;
        frame.pivotY=b.pivotY-(float)b.y;
        frame.duration=(i<4)?0.12f:(i<8?0.09f:0.10f);
        frame.visualBounds={0,0,(float)b.w,(float)b.h};
        result.push_back(frame);
    }
    return result;
}

void EnsureAnimator(Animator& a,StreetEnemyType type){
    if(a.texture.id!=0)return;
    const EnemyAnimationLayout layout=LayoutFor(type);
    Texture2D t=AssetManager::Get().GetTexture(TextureKeyFor(type));
    if(t.id!=0 && t.width==512 && t.height==384){
        a.Init(t,layout.columns,layout.rows,false);
        a.SetFrames(BuildSpriteFrames(type));
        a.Play({layout.idleStart,layout.idleEnd,0.12f,true});
        return;
    }
    if(t.id!=0)TraceLog(LOG_WARNING,"Distrito Fury: atlas rechazado para %s; se esperaba 512x384 y llegó %ix%i",TextureKeyFor(type),t.width,t.height);
}
void PlayIdle(Animator& animator,StreetEnemyType type){const EnemyAnimationLayout layout=LayoutFor(type);if(animator.isFinished||!animator.isPlaying||animator.currentFrame<layout.idleStart||animator.currentFrame>layout.idleEnd)animator.Play({layout.idleStart,layout.idleEnd,.12f,true});}
void PlayWalk(Animator& animator,StreetEnemyType type){const EnemyAnimationLayout layout=LayoutFor(type);if(animator.isFinished||!animator.isPlaying||animator.currentFrame<layout.walkStart||animator.currentFrame>layout.walkEnd)animator.Play({layout.walkStart,layout.walkEnd,.10f,true});}

void DrawFallbackEnemy(Vector2 p,const Stats&s,StreetEnemyType type,float scale,Color tint){
    const float w=s.bodyWidth*scale,h=s.bodyHeight*scale;
    const float cx=p.x, top=p.y-h, shoulder=top+h*.31f, hip=top+h*.64f;
    const float headR=w*.18f, torsoW=w*.42f, legW=w*.14f;
    const Color dark={20,23,27,255};
    DrawEllipse((int)p.x,(int)p.y,(int)(w*.34f),(int)(h*.07f),{0,0,0,145});
    if(type==StreetEnemyType::Punk){
        DrawCircle((int)cx,(int)(top+h*.18f),(int)headR,tint);
        DrawRectangle((int)(cx-torsoW*.5f),(int)shoulder,(int)torsoW,(int)(h*.34f),tint);
        DrawRectangle((int)(cx-torsoW*.72f),(int)(shoulder+h*.03f),(int)(torsoW*.24f),(int)(h*.30f),tint);
        DrawRectangle((int)(cx+torsoW*.48f),(int)(shoulder+h*.03f),(int)(torsoW*.24f),(int)(h*.30f),tint);
        DrawRectangle((int)(cx-legW*1.45f),(int)hip,(int)legW,(int)(h*.31f),dark);
        DrawRectangle((int)(cx+legW*.45f),(int)hip,(int)legW,(int)(h*.31f),dark);
        DrawRectangle((int)(cx-legW*1.75f),(int)(p.y-h*.04f),(int)(legW*1.7f),(int)(h*.05f),tint);
        DrawRectangle((int)(cx+legW*.15f),(int)(p.y-h*.04f),(int)(legW*1.7f),(int)(h*.05f),tint);
    } else if(type==StreetEnemyType::Brute||type==StreetEnemyType::Mutant||type==StreetEnemyType::ArmoredGuard){
        DrawCircle((int)cx,(int)(top+h*.17f),(int)(headR*1.12f),tint);
        DrawRectangle((int)(cx-torsoW*.62f),(int)shoulder,(int)(torsoW*1.24f),(int)(h*.37f),tint);
        DrawRectangle((int)(cx-torsoW*.92f),(int)shoulder,(int)(torsoW*.30f),(int)(h*.30f),tint);
        DrawRectangle((int)(cx+torsoW*.62f),(int)shoulder,(int)(torsoW*.30f),(int)(h*.30f),tint);
        DrawRectangle((int)(cx-legW*1.7f),(int)hip,(int)(legW*1.25f),(int)(h*.31f),dark);
        DrawRectangle((int)(cx+legW*.45f),(int)hip,(int)(legW*1.25f),(int)(h*.31f),dark);
        DrawRectangle((int)(cx-legW*1.95f),(int)(p.y-h*.04f),(int)(legW*1.8f),(int)(h*.05f),tint);
        DrawRectangle((int)(cx+legW*.25f),(int)(p.y-h*.04f),(int)(legW*1.8f),(int)(h*.05f),tint);
        if(type==StreetEnemyType::Mutant){DrawCircle((int)(cx-headR*.55f),(int)(top+h*.17f),3,{180,255,80,255});DrawCircle((int)(cx+headR*.55f),(int)(top+h*.17f),3,{180,255,80,255});}
    } else {
        DrawCircle((int)cx,(int)(top+h*.17f),(int)headR,tint);
        DrawRectangle((int)(cx-torsoW*.5f),(int)shoulder,(int)torsoW,(int)(h*.34f),tint);
        DrawRectangle((int)(cx-torsoW*.95f),(int)shoulder,(int)(torsoW*.30f),(int)(h*.28f),tint);
        DrawRectangle((int)(cx+torsoW*.65f),(int)shoulder,(int)(torsoW*.30f),(int)(h*.28f),tint);
        DrawRectangle((int)(cx-legW*1.4f),(int)hip,(int)legW,(int)(h*.31f),dark);
        DrawRectangle((int)(cx+legW*.4f),(int)hip,(int)legW,(int)(h*.31f),dark);
        DrawRectangle((int)(cx-legW*1.7f),(int)(p.y-h*.04f),(int)(legW*1.6f),(int)(h*.05f),tint);
        DrawRectangle((int)(cx+legW*.1f),(int)(p.y-h*.04f),(int)(legW*1.6f),(int)(h*.05f),tint);
    }
}
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
void StreetEnemy::TakeDamage(int damage,Vector3D knockback){
    if(!active||state==StreetEnemyState::Defeat)return;
    hp=std::max(0,hp-damage);velocity=knockback;
    const EnemyAnimationLayout layout=LayoutFor(type);
    if(hp==0){
        state=StreetEnemyState::Defeat;stateTimer=.85f;
        if(animator.texture.id!=0)animator.Play({layout.deathStart,layout.deathEnd,.14f,false});
        else{animator.isFinished=true;animator.isPlaying=false;}
    }else{
        state=StreetEnemyState::Hit;stateTimer=.30f;
        if(animator.texture.id!=0)animator.Play({layout.hitFrame,layout.hitFrame,.09f,false,{8,9}});
    }
}
void StreetEnemy::Draw() const{
    if(!active)return;
    const Stats s=GetStats(type);const Vector2 p=position.ToScreen();const float ds=DepthScale(position.y);
    const bool authored=animator.texture.id!=0&&!animator.frames.empty();
    const float visualScale=authored?s.scale*ds:ds;
    DrawEllipse((int)p.x,(int)p.y,26*s.scale*ds,8.5f*ds,{0,0,0,145});
    if(authored){
        Color tint=WHITE;
        if(state==StreetEnemyState::Hit)tint={255,225,225,255};
        if(state==StreetEnemyState::Defeat)tint={180,180,185,255};
        animator.Draw(p,visualScale,facing==Facing::Left,tint);
    }else{
        Color tint=state==StreetEnemyState::Hit?WHITE:state==StreetEnemyState::Defeat?Color{110,110,115,255}:s.fallbackTint;
        DrawFallbackEnemy(p,s,type,ds,tint);
    }
    if(state!=StreetEnemyState::Defeat){const int width=s.scale>=1.08f?82:68;const int x=(int)(p.x-width*.5f),y=(int)(p.y-s.bodyHeight*ds-11);DrawRectangle(x,y,width,6,{8,9,11,210});DrawRectangle(x,y,(int)(width*((float)hp/maxHp)),6,ThreatColor(type));}
}
}
