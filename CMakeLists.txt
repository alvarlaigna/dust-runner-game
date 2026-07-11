cmake_minimum_required(VERSION 3.16)

message([=[

██████╗░██╗░░░██╗░██████╗████████╗  ██████╗░██╗░░░██╗███╗░░██╗███╗░░██╗███████╗██████╗░
██╔══██╗██║░░░██║██╔════╝╚══██╔══╝  ██╔══██╗██║░░░██║████╗░██║████╗░██║██╔════╝██╔══██╗
██║░░██║██║░░░██║╚█████╗░░░░██║░░░  ██████╔╝██║░░░██║██╔██╗██║██╔██╗██║█████╗░░██████╔╝
██║░░██║██║░░░██║░╚═══██╗░░░██║░░░  ██╔══██╗██║░░░██║██║╚████║██║╚████║██╔══╝░░██╔══██╗
██████╔╝╚██████╔╝██████╔╝░░░██║░░░  ██║░░██║╚██████╔╝██║░╚███║██║░╚███║███████╗██║░░██║
╚═════╝░░╚═════╝░╚═════╝░░░░╚═╝░░░  ╚═╝░░╚═╝░╚═════╝░╚═╝░░╚══╝╚═╝░░╚══╝╚══════╝╚═╝░░╚═╝

        tactical desert survival  ·  https://alvarlaigna.itch.io/
]=])

project(DustRunner VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)

# Raylib via FetchContent (pinned to a release tag).
include(FetchContent)
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        6.0
    GIT_SHALLOW    TRUE
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Sources.
add_executable(DustRunner
    src/main.c
    src/terrain.c
    src/vehicle.c
    src/enemy.c
    src/projectile.c
    src/wave.c
    src/parts.c
    src/particle.c
    src/particle_draw.c
    src/hud.c
    src/postfx.c
    src/sky.c
    src/audio.c
    src/menu.c
    src/input.c
)

target_include_directories(DustRunner PRIVATE src)
target_link_libraries(DustRunner PRIVATE raylib)

# Platform link config.
if(EMSCRIPTEN)
    # Web: emit index.html (+ .js/.wasm) via the custom shell; GLFW3 + growable heap.
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    set_target_properties(DustRunner PROPERTIES OUTPUT_NAME "index")
    target_link_options(DustRunner PRIVATE
        "-sUSE_GLFW=3"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sGROWABLE_ARRAYBUFFERS=0"
        "-sFORCE_FILESYSTEM=1"
        "-sSTACK_SIZE=1048576"
        "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/web/shell.html")
    target_compile_definitions(DustRunner PRIVATE PLATFORM_WEB)
else()
    target_link_libraries(DustRunner PRIVATE m)
    if(UNIX AND NOT APPLE)
        target_link_libraries(DustRunner PRIVATE dl pthread)
    endif()
    if(WIN32)
        # Standalone exe: statically link the MinGW runtime so no DLLs are needed.
        target_link_options(DustRunner PRIVATE -static -static-libgcc)
    endif()
endif()

# Compiler flags (GCC / Clang / emcc).
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(DustRunner PRIVATE -O2)
    if(NOT EMSCRIPTEN)
        target_compile_options(DustRunner PRIVATE -s)   # strip: desktop only
    endif()
else()
    target_compile_options(DustRunner PRIVATE -g -Wall -Wextra)
endif()

if(EMSCRIPTEN)
    message(STATUS "Dust Runner v${PROJECT_VERSION}  ->  Web (WebAssembly)")
else()
    message(STATUS "Dust Runner v${PROJECT_VERSION}  ->  desktop (${CMAKE_SYSTEM_NAME})")
endif()

# particle.c stays raylib-call-free (ParticlesDraw lives in particle_draw.c instead) so
# this target never needs to link raylib.
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS AND NOT EMSCRIPTEN)
    add_executable(DustRunnerTests tests/test_parts.c src/parts.c tests/test_particle.c src/particle.c)
    target_include_directories(DustRunnerTests PRIVATE src ${raylib_SOURCE_DIR}/src)
    target_compile_options(DustRunnerTests PRIVATE -g -Wall -Wextra)
    if(NOT WIN32)
        target_link_libraries(DustRunnerTests PRIVATE m)
    endif()
    message(STATUS "Dust Runner tests: enabled")
endif()
