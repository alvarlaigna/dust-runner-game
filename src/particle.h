/*
 * Dust Runner - particle.h
 * Debris pool for destructible-ruin juice. Update is pure (no raylib calls).
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

/* Activate `count` shards at origin with random upward/outward velocities. */
void ParticlesSpawnBurst(Debris *pool, int max, Vector3 origin, int count);
/* Integrate gravity, ground bounce, shrink, expire. Pure - no raylib calls. */
void ParticlesUpdate(Debris *pool, int max, float dt);
/* Draw active shards as small cubes. */
void ParticlesDraw(const Debris *pool, int max);
