# Dust Runner

```text
:::::::::  :::    :::  :::::::: :::::::::::                        
:+:    :+: :+:    :+: :+:    :+:    :+:                            
+:+    +:+ +:+    +:+ +:+           +:+                            
+#+    +:+ +#+    +:+ +#++:++#++    +#+                            
+#+    +#+ +#+    +#+        +#+    +#+                            
#+#    #+# #+#    #+# #+#    #+#    #+#                            
#########   ########   ########     ###                            
:::::::::  :::    ::: ::::    ::: ::::    ::: :::::::::: ::::::::: 
:+:    :+: :+:    :+: :+:+:   :+: :+:+:   :+: :+:        :+:    :+:
+:+    +:+ +:+    +:+ :+:+:+  +:+ :+:+:+  +:+ +:+        +:+    +:+
+#++:++#:  +#+    +:+ +#+ +:+ +#+ +#+ +:+ +#+ +#++:++#   +#++:++#: 
+#+    +#+ +#+    +#+ +#+  +#+#+# +#+  +#+#+# +#+        +#+    +#+
#+#    #+# #+#    #+# #+#   #+#+# #+#   #+#+# #+#        #+#    #+#
###    ###  ########  ###    #### ###    #### ########## ###    ###
```

**The commander never touches the wheel.**

A tactical vehicle-survival game. Command an armored 4×4 through a procedural desert
wasteland using point-and-click orders. Survive escalating waves, collect scrap, and
choose upgrades between desperate stands in this HD-2D diorama experience.

*Vampire Survivors × Cannon Fodder × FTL*

**Play in your browser:** https://alvarlaigna.itch.io/dust-runner

**Status:** playable prototype, single-player, programmer-art visuals. Fixed 720×720,
runs on desktop (Windows / Linux) and in the browser via WebAssembly.

## Build

Plain C99 and [raylib 6.0](https://www.raylib.com), fetched and built automatically by
CMake `FetchContent` on first configure (needs internet the first time).

### Prerequisites

- CMake 3.16+ and Ninja
- A GCC or Clang toolchain (MinGW-w64 on Windows). MSVC is not supported: the build
  uses GCC-style flags.
- For the web build, the [Emscripten SDK](https://emscripten.org) (`emsdk`).

### Desktop

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release   # or Debug
cmake --build build
./build/DustRunner        # Linux / macOS
./build/DustRunner.exe    # Windows
```

Debug builds with `-Wall -Wextra` and stays warning-clean. Release adds `-O2` and
strips symbols; the Windows binary is statically linked, so it needs no MinGW DLLs.

> The `build/` directory is git-ignored and not portable between operating systems.
> Delete and reconfigure if you switch platforms.

### Web (WebAssembly)

Activate emsdk in your shell, then:

```sh
emcmake cmake -S . -B build-web -G Ninja -DPLATFORM=Web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web
python -m http.server 8000 --directory build-web   # open http://localhost:8000/index.html
```

The web build emits `index.html` + `index.js` + `index.wasm` (about 430 KB total). Zip
those and upload as an HTML5 game on itch.io.

### Linux dependencies

```sh
sudo apt install build-essential cmake ninja-build libasound2-dev libx11-dev \
  libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev
```

CI (`.github/workflows/build.yml`) builds all three targets on every push.

## Play

You are the commander, not the driver. Issue movement orders with the left mouse button
and the vehicle drives itself with realistic steering and terrain-aware movement. The
turret auto-targets the nearest threat, or take manual control for critical shots.
Position carefully, survive the waves, choose upgrades, repeat.

## Controls

- **Left click** - move the vehicle to a point (also drives menus and tap-to-move on touch)
- **Right click** - manual turret fire toward the cursor
- **SPACE** - toggle turret auto-aim
- **ESC** - pause (Continue / Settings / Quit to Title)
- **R** - restart on the game-over screen

On touch devices auto-aim defaults on, with on-screen auto-aim and pause buttons.

## Features

- Point-and-click command: indirect control creates tactical tension
- Autonomous turret with manual override
- Wave-based survival with escalating enemy types
- Terrain that affects movement: hardpan, sand, dune, ruin
- Weapon / chassis / utility upgrades between waves
- HD-2D diorama look: tilt-shift post-processing, procedural sky, parallax horizon
- Fully procedural audio: synthesized SFX plus engine and wind layers
- Desktop and mobile-friendly WebAssembly on a single 720×720 canvas

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

Source is released under the zlib/libpng license.

Copyright (c) 2026 Alvar Laigna (https://alvarlaigna.itch.io/)
