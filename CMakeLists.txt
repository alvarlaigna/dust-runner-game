cmake_minimum_required(VERSION 3.16)
project(DustRunner C)

set(CMAKE_C_STANDARD 99)

# ── Raylib via FetchContent ──────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        6.0
    GIT_SHALLOW    TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# ── Sources ──────────────────────────────────────────────────────────────────
add_executable(DustRunner
    src/main.c
    src/terrain.c
    src/vehicle.c
    src/enemy.c
    src/projectile.c
    src/wave.c
    src/upgrade.c
    src/hud.c
    src/postfx.c
    src/sky.c
    src/audio.c
    src/menu.c
)

target_include_directories(DustRunner PRIVATE src)
target_link_libraries(DustRunner PRIVATE raylib m)

if(UNIX AND NOT APPLE)
    target_link_libraries(DustRunner PRIVATE dl pthread)
endif()

# ── Compiler flags ───────────────────────────────────────────────────────────
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(DustRunner PRIVATE -O2 -s)
else()
    target_compile_options(DustRunner PRIVATE -g -Wall -Wextra)
endif()
