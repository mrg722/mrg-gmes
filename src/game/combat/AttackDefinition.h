#pragma once

namespace district_fury {

enum class AttackId {
    Punch,
    Kick,
    EnergyWave,
    DashAttack,
    RageAttack,
    Finisher
};

struct AttackDefinition {
    AttackId id;
    const char* name;
    int damage;
    float startup;
    float activeStart;
    float activeEnd;
    float recovery;
    float duration;
    float range;
    float depthRange;
    float knockback;
    float hitstun;
    int spCost;
    int rageGain;
    int priority;
    float cooldown;
    bool canCombo;
    bool canCancel;
    bool guardBreak;
};

}
