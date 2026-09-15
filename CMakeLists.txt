cmake_minimum_required(VERSION 3.21)

project(DistrictFury VERSION 0.3.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG c1ab645ca298a2801097931d1079b10ff7eb9df8
)
FetchContent_MakeAvailable(raylib)

add_library(district_fury_core
    src/audio/AudioSystem.cpp
    src/core/ApplicationState.cpp
    src/game/Player.cpp
    src/game/StreetEnemy.cpp
    src/game/Scene.cpp
    src/game/GameManager.cpp
    src/game/ProductionGame.cpp
    src/game/Stage1StoryGame.cpp
    src/game/Stage2Game.cpp
    src/game/Stage3Game.cpp
    src/rendering/AssetManager.cpp
)
target_include_directories(district_fury_core PUBLIC src)
target_link_libraries(district_fury_core PUBLIC raylib)

add_executable(district_fury src/main.cpp)
target_link_libraries(district_fury PRIVATE district_fury_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(application_state_tests tests/ApplicationStateTests.cpp)
    target_link_libraries(application_state_tests PRIVATE district_fury_core)
    add_test(NAME application_state_tests COMMAND application_state_tests)

    add_executable(combat_smoke_tests tests/CombatSmokeTests.cpp)
    target_link_libraries(combat_smoke_tests PRIVATE district_fury_core)
    add_test(NAME combat_smoke_tests COMMAND combat_smoke_tests)
endif()
