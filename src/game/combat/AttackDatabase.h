#pragma once
#include "game/combat/AttackDefinition.h"

namespace district_fury {

class AttackDatabase {
public:
    static const AttackDefinition& Get(AttackId id);
};

}
