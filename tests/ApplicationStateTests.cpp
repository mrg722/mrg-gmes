#include "core/ApplicationState.h"
#include "game/Types.h"

#include <cstdlib>
#include <iostream>

int main() {
    using district_fury::CombatBox;
    using district_fury::core::ApplicationState;
    using district_fury::core::shouldContinue;

    if (!shouldContinue(ApplicationState::Running)) {
        std::cerr << "A running application must continue.\n";
        return EXIT_FAILURE;
    }

    if (shouldContinue(ApplicationState::ExitRequested)) {
        std::cerr << "An exit request must stop the application.\n";
        return EXIT_FAILURE;
    }

    const CombatBox player = {100.0f, 100.0f, 50.0f, 100.0f};
    const CombatBox attack = {130.0f, 140.0f, 80.0f, 35.0f};
    const CombatBox miss = {220.0f, 260.0f, 40.0f, 40.0f};

    if (!player.Intersects(attack)) {
        std::cerr << "Overlapping combat boxes must intersect.\n";
        return EXIT_FAILURE;
    }

    if (player.Intersects(miss)) {
        std::cerr << "Separated combat boxes must not intersect.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
