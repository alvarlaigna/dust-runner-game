/*
 * Dust Runner - terrain.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

void  TerrainGenerate(Terrain *t, unsigned int seed);
void  TerrainDraw(const Terrain *t, Vector3 center);
float TerrainSpeedMod(const Terrain *t, Vector3 pos);
bool  TerrainIsPassable(const Terrain *t, Vector3 pos);
