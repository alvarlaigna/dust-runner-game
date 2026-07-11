# Dust Runner

**The commander never touches the wheel.**

A tactical vehicle-survival game. Command an armored 4×4 through a procedural desert wasteland using point-and-click orders. Survive escalating waves of enemies, collect scrap, and choose upgrades between desperate stands in this HD-2D diorama experience.

*Vampire Survivors × Cannon Fodder × FTL*

**Current status:** Playable prototype. Single-player. Programmer-art visuals.

## Getting Started with this template

This project is built with plain C99 and [raylib 6.0](https://www.raylib.com). Raylib is automatically downloaded and built via CMake's `FetchContent` on first configure.

### Prerequisites
- CMake 3.16 or newer
- A C99-compatible compiler (GCC, Clang, or MSVC)
- Internet connection for the first build (to fetch raylib)

### Build & Run (CMake)

```sh
# Configure (first time will download and build raylib ~40s)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build

# Run
./build/DustRunner          # Linux / macOS
./build/DustRunner.exe      # Windows
```

**Release build:**
```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

> **Note:** The `build/` directory is git-ignored and not portable between operating systems. Delete and reconfigure if you switch platforms.

### Windows (Visual Studio)
1. Generate the solution:
   ```powershell
   cmake -S . -B build -G "Visual Studio 17 2022"
   ```
2. Open `build/DustRunner.sln`
3. Set `DustRunner` as the startup project and run.

### Linux Dependencies
Install development packages for your distro (example for Debian/Ubuntu):
```sh
sudo apt install build-essential cmake libasound2-dev libx11-dev libxrandr-dev \
  libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev libxxf86vm-dev
```

## Description

You are the commander, not the driver.

Issue movement orders to your armored vehicle with the left mouse button. The vehicle handles driving on its own using realistic steering and terrain-aware movement. Your turret automatically targets the nearest threat or take manual control with right-click for critical shots.

Position carefully. Terrain affects your speed. Survive the waves. Choose upgrades. Repeat.

## Features

- **Point-and-click command system** — indirect control creates real tactical tension
- **Autonomous turret with manual override** — auto-aim for normal play, right-click for clutch shots
- **Wave-based survival** — escalating enemy types and increasing pressure
- **Meaningful terrain** — hardpan, sand, dunes, and ruins directly affect movement and strategy
- **Upgrade system** — weapon, chassis, and utility upgrades between waves (stackable)
- **Stylized HD-2D diorama visuals** — fixed camera, tilt-shift post-processing, procedural sky and parallax background
- **Fully procedural audio** — synthesized sound effects and engine/wind layers

## Controls

**Keyboard + Mouse:**

- **Left Mouse Button** — Set movement target for the vehicle
- **Right Mouse Button** — Manual turret fire toward cursor (one shot)
- **SPACE** — Toggle turret auto-aim on/off
- **ESC** — Open pause menu (settings, volume, quit)
- **R** — Restart (on game over screen)

## Screenshots

![Dust Runner screenshot](https://img.itch.zone/aW1nLzI4NDE2MTM2LnBuZw==/315x250%23c/akDO1v.png)

## Developers

**Alvar Laigna** — Design, Programming, Audio, Visuals  
https://alvarlaigna.com/

*Note: LLM assistance was used while coding this game.*

## Links

- Author: [https://alvarlaigna.com/](https://alvarlaigna.com/)
- itch.io Release: [https://alvarlaigna.itch.io/dust-runner](https://alvarlaigna.itch.io/dust-runner)

## License

This project sources are licensed under an unmodified zlib/libpng license, which is an OSI-certified, BSD-like license that allows static linking with closed source software.

Copyright (c) 2026 Alvar Laigna (https://alvarlaigna.com/)