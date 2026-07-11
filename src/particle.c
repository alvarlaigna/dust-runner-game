/*
 * Dust Runner - particle.c
 * Pure debris-pool logic (spawn + update). No raylib calls, so this file links
 * into the DustRunnerTests target without pulling in raylib. ParticlesDraw, the
 * one raylib-touching function, lives in particle_draw.c instead.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "particle.h"
#include <stdlib.h>

static float Rand11(void) { return (float)rand() / (float)RAND_MAX * 2.0f - 1.0f; }

void ParticlesSpawnBurst(Debris *pool, int max, Vector3 origin, int count) {
    int spawned = 0;
    for (int i = 0; i < max && spawned < count; i++) {
        if (pool[i].active) continue;
        pool[i].active = true;
        pool[i].pos = origin;
        pool[i].vel = (Vector3){ Rand11() * 6.0f, 4.0f + Rand11() * 3.0f + 3.0f, Rand11() * 6.0f };
        pool[i].size = 0.35f + (Rand11() + 1.0f) * 0.15f;
        pool[i].life = 1.0f + (Rand11() + 1.0f) * 0.4f;
        spawned++;
    }
}

void ParticlesUpdate(Debris *pool, int max, float dt) {
    for (int i = 0; i < max; i++) {
        if (!pool[i].active) continue;
        pool[i].vel.y -= DEBRIS_GRAVITY * dt;
        pool[i].pos.x += pool[i].vel.x * dt;
        pool[i].pos.y += pool[i].vel.y * dt;
        pool[i].pos.z += pool[i].vel.z * dt;
        if (pool[i].pos.y < 0.0f) {                     /* ground bounce */
            pool[i].pos.y = 0.0f;
            pool[i].vel.y = -pool[i].vel.y * DEBRIS_BOUNCE;
            pool[i].vel.x *= 0.6f; pool[i].vel.z *= 0.6f;
        }
        pool[i].life -= dt;
        pool[i].size -= dt * 0.25f;
        if (pool[i].life <= 0.0f || pool[i].size <= 0.0f) pool[i].active = false;
    }
}
