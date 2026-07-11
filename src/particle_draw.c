/*
 * Dust Runner - particle_draw.c
 * ParticlesDraw only: the one raylib-touching piece of the debris pool, split
 * out of particle.c so DustRunnerTests can link the pure pool logic without
 * raylib. See particle.h for the shared API.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "particle.h"

void ParticlesDraw(const Debris *pool, int max) {
    for (int i = 0; i < max; i++) {
        if (!pool[i].active) continue;
        float s = pool[i].size;
        DrawCube(pool[i].pos, s, s, s, (Color){ 90, 80, 66, 255 });
    }
}
