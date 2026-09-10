cmake_minimum_required(VERSION 3.21)

project(DistrictFury VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# raylib 5.5 is deliberately pinned to its release commit so every clean
# configuration resolves the same upstream source without a system install.
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG c1ab645ca298a2801097931d1079b10ff7eb9df8 # raylib 5.5
)
FetchContent_MakeAvailable(raylib)

add_library(district_fury_core
    src/core/ApplicationState.cpp
)
target_include_directories(district_fury_core PUBLIC src)

add_executable(district_fury src/main.cpp)
target_link_libraries(district_fury PRIVATE district_fury_core raylib)

include(CTest)
if(BUILD_TESTING)
    add_executable(application_state_tests tests/ApplicationStateTests.cpp)
    target_link_libraries(application_state_tests PRIVATE district_fury_core)
    add_test(NAME application_state_tests COMMAND application_state_tests)
endif()
