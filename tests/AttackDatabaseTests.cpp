#include "game/combat/AttackDatabase.h"
#include <cassert>

using namespace district_fury;

int main() {
    const auto& punch = AttackDatabase::Get(AttackId::Punch);
    const auto& kick = AttackDatabase::Get(AttackId::Kick);
    const auto& energy = AttackDatabase::Get(AttackId::EnergyWave);
    const auto& dash = AttackDatabase::Get(AttackId::DashAttack);
    const auto& rage = AttackDatabase::Get(AttackId::RageAttack);
    const auto& finisher = AttackDatabase::Get(AttackId::Finisher);

    assert(punch.damage > 0);
    assert(kick.damage > punch.damage);
    assert(energy.spCost == 20);
    assert(energy.activeStart >= energy.startup);
    assert(energy.activeEnd <= energy.duration);
    assert(dash.knockback > punch.knockback);
    assert(rage.priority > punch.priority);
    assert(finisher.damage > rage.damage);
    assert(finisher.spCost > energy.spCost);
    assert(finisher.guardBreak);
    assert(!energy.canCombo);

    return 0;
}
