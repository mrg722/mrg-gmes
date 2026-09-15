#include "game/StreetEnemy.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
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

// Mide cada celda del atlas directamente desde su alpha. Esto evita mantener una
// tabla de bounds dependiente del enemigo y elimina el riesgo de reutilizar la
// geometria de otro personaje.
std::vector<SpriteFrame> BuildSpriteFrames(Texture2D texture){
    std::vector<SpriteFrame> result;
    if(texture.id==0||texture.width!=512||texture.height!=384)return result;

    Image image=LoadImageFromTexture(texture);
    if(image.data==nullptr)return result;
    ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    if(image.data==nullptr||image.width!=512||image.height!=384){UnloadImage(image);return result;}

    const Color* pixels=static_cast<const Color*>(image.data);
    result.reserve(12);
    for(int frameIndex=0;frameIndex<12;++frameIndex){
        const int cellX=(frameIndex%4)*128;
        const int cellY=(frameIndex/4)*128;
        int minX=128,minY=128,maxX=-1,maxY=-1;
        std::vector<int> lowerXs;
        lowerXs.reserve(128*16);

        for(int y=0;y<128;++y){
            for(int x=0;x<128;++x){
                const Color& pixel=pixels[(cellY+y)*image.width+(cellX+x)];
                if(pixel.a<8)continue;
                minX=std::min(minX,x);minY=std::min(minY,y);
                maxX=std::max(maxX,x);maxY=std::max(maxY,y);
            }
        }
        if(maxX<minX||maxY<minY){
            TraceLog(LOG_WARNING,"Distrito Fury: frame %i del atlas enemigo no contiene pixeles alpha",frameIndex);
            continue;
        }

        // El ancla horizontal se calcula con la zona inferior del personaje,
        // no con toda la silueta. Así un arma, cadena o efecto que sobresalga
        // durante un ataque no mueve el punto de apoyo de los pies.
        const int lowerStart=std::max(minY,maxY-15);
        for(int y=lowerStart;y<=maxY;++y){
            for(int x=minX;x<=maxX;++x){
                const Color& pixel=pixels[(cellY+y)*image.width+(cellX+x)];
                if(pixel.a>=8)lowerXs.push_back(x);
            }
        }
        if(lowerXs.empty()){
            for(int y=minY;y<=maxY;++y){
                for(int x=minX;x<=maxX;++x){
                    const Color& pixel=pixels[(cellY+y)*image.width+(cellX+x)];
                    if(pixel.a>=8)lowerXs.push_back(x);
                }
            }
        }
        std::sort(lowerXs.begin(),lowerXs.end());
        const float pivotX=static_cast<float>(lowerXs[lowerXs.size()/2]);
        const float pivotY=static_cast<float>(maxY+1);

        SpriteFrame frame;
        frame.source={(float)(cellX+minX),(float)(cellY+minY),(float)(maxX-minX+1),(float)(maxY-minY+1)};
        frame.width=(float)(maxX-minX+1);
        frame.height=(float)(maxY-minY+1);
        frame.pivotX=pivotX-(float)minX;
        frame.pivotY=pivotY-(float)minY;
        frame.duration=(frameIndex<4)?0.12f:(frameIndex<8?0.09f:0.10f);
        frame.visualBounds={0,0,frame.width,frame.height};
        result.push_back(frame);
    }
    UnloadImage(image);
    return result.size()==12?result:std::vector<SpriteFrame>{};
}

void EnsureAnimator(Animator& a,StreetEnemyType type){
    if(a.texture.id!=0)return;
    const EnemyAnimationLayout layout=LayoutFor(type);
    Texture2D t=AssetManager::Get().GetTexture(TextureKeyFor(type));
    if(t.id!=0 && t.width==512 && t.height==384){
        std::vector<SpriteFrame> measured=BuildSpriteFrames(t);
        if(measured.size()==12){
            a.Init(t,layout.columns,layout.rows,false);
            a.SetFrames(std::move(measured));
            a.Play({layout.idleStart,layout.idleEnd,0.12f,true});
            return;
        }
        TraceLog(LOG_WARNING,"Distrito Fury: atlas enemigo rechazado para %s porque sus 12 frames no pudieron medirse",TextureKeyFor(type));
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
