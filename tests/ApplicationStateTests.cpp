#include "core/ApplicationState.h"

#include <cstdlib>
#include <iostream>

int main() {
    using district_fury::core::ApplicationState;
    using district_fury::core::shouldContinue;

    if (!shouldContinue(ApplicationState::Running)) {
        std::cerr << "A running application must continue.
";
        return EXIT_FAILURE;
    }

    if (shouldContinue(ApplicationState::ExitRequested)) {
        std::cerr << "An exit request must stop the application.
";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
