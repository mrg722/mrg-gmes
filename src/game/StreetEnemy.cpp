#include "game/StreetEnemy.h"
#include "rendering/AssetManager.h"
#include <algorithm>
#include <cmath>

namespace district_fury {
namespace {

struct Stats {
    int hp;
    float speed;
    float damage;
    float range;
    float depth;
    float attackDuration;
    float scale;
    Color fallbackTint;
    float bodyWidth;
    float bodyHeight;
};

Stats GetStats(StreetEnemyType type) {
    switch (type) {
        case StreetEnemyType::Brute:
            return {130, 95.0f, 20.0f, 132.0f, 46.0f, 0.72f, 1.10f,
                    {220, 140, 70, 255}, 64.0f, 112.0f};
        case StreetEnemyType::Charger:
            return {72, 220.0f, 14.0f, 120.0f, 42.0f, 0.54f, 1.00f,
                    {90, 155, 220, 255}, 54.0f, 108.0f};
        case StreetEnemyType::Enforcer:
            return {190, 122.0f, 24.0f, 146.0f, 50.0f, 0.76f, 1.12f,
                    {235, 185, 70, 255}, 66.0f, 116.0f};
        case StreetEnemyType::Punk:
        default:
            return {58, 172.0f, 12.0f, 110.0f, 38.0f, 0.62f, 1.00f,
                    {225, 65, 80, 255}, 54.0f, 108.0f};
    }
}

Color ThreatColor(StreetEnemyType type) {
    switch (type) {
        case StreetEnemyType::Brute: return {220, 140, 70, 255};
        case StreetEnemyType::Charger: return {90, 155, 220, 255};
        case StreetEnemyType::Enforcer: return {235, 185, 70, 255};
        case StreetEnemyType::Punk:
        default: return {225, 65, 80, 255};
    }
}

float DepthScale(float laneY) {
    const float t = std::clamp(
        (laneY - kLaneMinY) / (kLaneMaxY - kLaneMinY),
        0.0f,
        1.0f
    );
    return 0.86f + 0.26f * t;
}

const char* TextureKeyFor(StreetEnemyType type) {
    switch (type) {
        case StreetEnemyType::Brute: return "brute_clean";
        case StreetEnemyType::Charger: return "charger_clean";
        case StreetEnemyType::Enforcer: return "enforcer_clean";
        case StreetEnemyType::Punk:
        default: return "punk_clean";
    }
}

void EnsureAnimator(Animator& animator, StreetEnemyType type) {
    if (animator.texture.id != 0) return;

    const Texture2D texture = AssetManager::Get().GetTexture(TextureKeyFor(type));
    if (texture.id != 0) {
        animator.Init(texture, 4, 3, true);
        animator.Play({0, 3, 0.12f, true});
        return;
    }
}

void DrawFallbackEnemy(Vector2 screenPos, const Stats& stats, StreetEnemyType type,
                       float scale, Color tint) {
    const float width = stats.bodyWidth * scale;
    const float height = stats.bodyHeight * scale;
    const int x = static_cast<int>(screenPos.x - width * 0.5f);
    const int y = static_cast<int>(screenPos.y - height);
    const int bodyWidth = static_cast<int>(width);
    const int bodyHeight = static_cast<int>(height * 0.62f);
    const int bodyY = y + static_cast<int>(height * 0.32f);

    if (type == StreetEnemyType::Punk) {
        DrawTriangle({static_cast<float>(x + bodyWidth / 2), static_cast<float>(y)},
                      {static_cast<float>(x + bodyWidth), static_cast<float>(bodyY)},
                      {static_cast<float>(x), static_cast<float>(bodyY)}, tint);
        DrawRectangle(x + bodyWidth / 4, bodyY, bodyWidth / 2, bodyHeight, tint);
    } else if (type == StreetEnemyType::Charger) {
        DrawRectangle(x + bodyWidth / 5, bodyY, bodyWidth * 3 / 5, bodyHeight, tint);
        DrawLine(x + bodyWidth / 5, bodyY + bodyHeight, x, static_cast<int>(screenPos.y), tint);
        DrawLine(x + bodyWidth * 4 / 5, bodyY + bodyHeight, x + bodyWidth,
                 static_cast<int>(screenPos.y), tint);
    } else if (type == StreetEnemyType::Brute) {
        DrawRectangle(x, bodyY, bodyWidth, bodyHeight, tint);
        DrawCircle(x + bodyWidth / 2, y + static_cast<int>(height * 0.2f),
                   bodyWidth * 0.24f, tint);
    } else {
        DrawRectangle(x + bodyWidth / 8, bodyY, bodyWidth * 3 / 4, bodyHeight, tint);
        DrawRectangle(x + bodyWidth / 4, y, bodyWidth / 2, static_cast<int>(height * 0.32f), tint);
        DrawRectangle(x, bodyY + bodyHeight / 4, bodyWidth / 6, bodyHeight / 2, tint);
        DrawRectangle(x + bodyWidth * 5 / 6, bodyY + bodyHeight / 4,
                      bodyWidth / 6, bodyHeight / 2, tint);
    }
}

}

StreetEnemy::StreetEnemy() {
    Init({900.0f, 560.0f, 0.0f}, StreetEnemyType::Punk);
}

void StreetEnemy::Init(Vector3D startPos, StreetEnemyType enemyType) {
    const Stats stats = GetStats(enemyType);

    position = startPos;
    velocity = {0.0f, 0.0f, 0.0f};
    facing = Facing::Left;
    state = StreetEnemyState::Idle;
    type = enemyType;
    active = false;

    hp = stats.hp;
    maxHp = stats.hp;
    moveSpeed = stats.speed;
    attackDamage = stats.damage;
    attackRange = stats.range;
    attackDepth = stats.depth;
    attackDuration = stats.attackDuration;
    stateTimer = 0.0f;
    attackElapsed = 0.0f;
    hasHit = false;
    animator = Animator{};
}

void StreetEnemy::Activate() {
    active = true;
    if (state == StreetEnemyState::Defeat) return;
    state = StreetEnemyState::Idle;
    hasHit = false;
}

void StreetEnemy::Update(float dt, const Player& player) {
    if (!active) return;

    EnsureAnimator(animator, type);
    animator.Update(dt);

    if (state == StreetEnemyState::Defeat) {
        stateTimer -= dt;
        return;
    }

    if (state == StreetEnemyState::Hit) {
        stateTimer -= dt;
        position.x += velocity.x * dt;
        position.y += velocity.y * dt;
        velocity.x *= 0.86f;
        velocity.y *= 0.86f;

        if (stateTimer <= 0.0f) {
            state = StreetEnemyState::Idle;
            animator.Play({0, 3, 0.12f, true});
        }

        position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.0f);
        position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
        return;
    }

    if (state == StreetEnemyState::Attack) {
        stateTimer -= dt;
        attackElapsed += dt;

        if (stateTimer <= 0.0f || animator.isFinished) {
            state = StreetEnemyState::Idle;
            animator.Play({0, 3, 0.12f, true});
        }
        return;
    }

    if (player.state == PlayerState::Defeat) return;

    const float dx = player.position.x - position.x;
    const float dy = player.position.y - position.y;
    const float horizontal = std::abs(dx);
    const float depth = std::abs(dy);
    const float distance = std::sqrt(dx * dx + dy * dy);

    facing = dx >= 0.0f ? Facing::Right : Facing::Left;

    if (horizontal < attackRange && depth < attackDepth) {
        state = StreetEnemyState::Attack;
        stateTimer = attackDuration;
        attackElapsed = 0.0f;
        hasHit = false;
        animator.Play({4, 6, type == StreetEnemyType::Charger ? 0.075f : 0.10f, false});
        return;
    }

    if (distance < 760.0f) {
        state = StreetEnemyState::Chase;
        if (distance > 0.001f) {
            position.x += (dx / distance) * moveSpeed * dt;
            position.y += (dy / distance) * moveSpeed * 0.72f * dt;
        }
        if (animator.isFinished || !animator.isPlaying || animator.currentFrame > 3) {
            animator.Play({0, 3, 0.12f, true});
        }
    } else {
        state = StreetEnemyState::Idle;
        if (animator.isFinished || animator.currentFrame > 3) {
            animator.Play({0, 3, 0.12f, true});
        }
    }

    position.x = std::clamp(position.x, kStageStartX, kStageEndX - 90.0f);
    position.y = std::clamp(position.y, kLaneMinY, kLaneMaxY);
}

bool StreetEnemy::AttackIsActive() const {
    return state == StreetEnemyState::Attack &&
           attackElapsed >= attackDuration * 0.34f &&
           attackElapsed <= attackDuration * 0.74f;
}

bool StreetEnemy::IsDefeated() const {
    return state == StreetEnemyState::Defeat;
}

CombatBox StreetEnemy::GetHurtbox() const {
    if (!active || IsDefeated()) return {};
    const Stats stats = GetStats(type);
    return {
        position.x - stats.bodyWidth * 0.5f,
        position.y - stats.bodyHeight,
        stats.bodyWidth,
        stats.bodyHeight
    };
}

CombatBox StreetEnemy::GetAttackHitbox() const {
    if (!AttackIsActive()) return {};

    const float direction = facing == Facing::Right ? 1.0f : -1.0f;
    const float width = attackRange * 0.78f;
    const float height = 52.0f;
    const float centerX = position.x + direction * attackRange * 0.55f;
    const float centerY = position.y - 66.0f;

    return {centerX - width * 0.5f, centerY - height * 0.5f, width, height};
}

const char* StreetEnemy::GetTypeName() const {
    switch (type) {
        case StreetEnemyType::Brute: return "BRUTE";
        case StreetEnemyType::Charger: return "CHARGER";
        case StreetEnemyType::Enforcer: return "ENFORCER";
        case StreetEnemyType::Punk:
        default: return "PUNK";
    }
}

void StreetEnemy::TakeDamage(int damage, Vector3D knockback) {
    if (!active || state == StreetEnemyState::Defeat) return;

    hp = std::max(0, hp - damage);
    velocity = knockback;

    if (hp == 0) {
        state = StreetEnemyState::Defeat;
        stateTimer = 0.85f;
        if (animator.texture.id != 0) {
            // Frames 10-11 are the authored defeat pair in the 4x3 enemy atlas.
            animator.Play({10, 11, 0.14f, false});
        } else {
            animator.isFinished = true;
            animator.isPlaying = false;
        }
    } else {
        state = StreetEnemyState::Hit;
        stateTimer = 0.38f;
        if (animator.texture.id != 0) animator.Play({0, 0, 0.09f, false});
    }
}

void StreetEnemy::Draw() const {
    if (!active) return;

    const Stats stats = GetStats(type);
    const Vector2 screenPos = position.ToScreen();
    const float depthScale = DepthScale(position.y);
    const float visualScale = (animator.normalizedAtlas ? stats.scale : 0.86f) * depthScale;

    DrawEllipse(
        static_cast<int>(screenPos.x),
        static_cast<int>(screenPos.y),
        26.0f * stats.scale * depthScale,
        8.5f * depthScale,
        {0, 0, 0, 145}
    );

    if (animator.texture.id != 0) {
        Color tint = animator.normalizedAtlas ? WHITE : stats.fallbackTint;
        if (state == StreetEnemyState::Hit) tint = {255, 215, 215, 255};
        if (state == StreetEnemyState::Defeat) tint = {175, 175, 175, 255};
        animator.Draw(screenPos, visualScale, facing == Facing::Left, tint);
    } else {
        const Color fallbackTint = state == StreetEnemyState::Hit ? WHITE
            : state == StreetEnemyState::Defeat ? Color{110, 110, 115, 255}
            : stats.fallbackTint;
        DrawFallbackEnemy(screenPos, stats, type, depthScale, fallbackTint);
    }

    if (state != StreetEnemyState::Defeat) {
        const int width = stats.scale >= 1.08f ? 82 : 68;
        const int x = static_cast<int>(screenPos.x - width * 0.5f);
        const int y = static_cast<int>(screenPos.y - stats.bodyHeight * depthScale - 11.0f);

        DrawRectangle(x, y, width, 6, {8, 9, 11, 210});
        DrawRectangle(
            x,
            y,
            static_cast<int>(width * (static_cast<float>(hp) / static_cast<float>(maxHp))),
            6,
            ThreatColor(type)
        );
    }
}

}
