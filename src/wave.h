/*
 * Dust Runner - wave.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

void WaveInit(WaveState *w);
void WaveUpdate(WaveState *w, Enemy *pool, int poolMax, const Vehicle *v, float dt);
bool WaveIsComplete(const WaveState *w, const Enemy *pool, int poolMax);
void WaveStartNext(WaveState *w);
