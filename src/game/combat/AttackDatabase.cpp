#include "game/combat/AttackDatabase.h"

namespace district_fury {
namespace {
const AttackDefinition kPunch{
    AttackId::Punch, "PUNCH", 10, 0.08f, 0.08f, 0.23f, 0.07f, 0.30f,
    115.0f, 42.0f, 300.0f, 0.16f, 0, 8, 1, 0.0f, true, true, false
};
const AttackDefinition kKick{
    AttackId::Kick, "KICK", 14, 0.10f, 0.10f, 0.29f, 0.07f, 0.36f,
    135.0f, 42.0f, 420.0f, 0.22f, 0, 10, 2, 0.0f, true, true, false
};
const AttackDefinition kEnergy{
    AttackId::EnergyWave, "ENERGY WAVE", 18, 0.22f, 0.22f, 0.46f, 0.08f, 0.54f,
    220.0f, 55.0f, 500.0f, 0.24f, 20, 4, 3, 0.10f, false, false, true
};
const AttackDefinition kDashAttack{
    AttackId::DashAttack, "DASH ATTACK", 16, 0.05f, 0.05f, 0.16f, 0.08f, 0.26f,
    125.0f, 48.0f, 380.0f, 0.18f, 0, 10, 3, 0.20f, true, true, false
};
const AttackDefinition kRageAttack{
    AttackId::RageAttack, "RAGE ATTACK", 24, 0.10f, 0.10f, 0.30f, 0.10f, 0.42f,
    145.0f, 55.0f, 520.0f, 0.28f, 0, 0, 4, 0.20f, true, true, false
};
const AttackDefinition kFinisher{
    AttackId::Finisher, "FINISHER", 40, 0.18f, 0.18f, 0.52f, 0.16f, 0.76f,
    175.0f, 65.0f, 700.0f, 0.45f, 40, 0, 5, 0.70f, false, false, true
};
}

const AttackDefinition& AttackDatabase::Get(AttackId id) {
    switch (id) {
        case AttackId::Kick: return kKick;
        case AttackId::EnergyWave: return kEnergy;
        case AttackId::DashAttack: return kDashAttack;
        case AttackId::RageAttack: return kRageAttack;
        case AttackId::Finisher: return kFinisher;
        default: return kPunch;
    }
}
}
