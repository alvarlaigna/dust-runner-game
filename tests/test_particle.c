/*
 * Dust Runner - tests/test_particle.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "particle.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void run_particle_tests(void) {
    Debris pool[DEBRIS_MAX]; memset(pool, 0, sizeof pool);

    ParticlesSpawnBurst(pool, DEBRIS_MAX, (Vector3){ 5, 0, 5 }, DEBRIS_PER_SMASH);
    int n = 0; for (int i = 0; i < DEBRIS_MAX; i++) if (pool[i].active) n++;
    assert(n == DEBRIS_PER_SMASH);

    /* a shard rises then falls under gravity and bounces at y=0 */
    memset(pool, 0, sizeof pool);
    pool[0].active = true; pool[0].pos = (Vector3){0, 1.0f, 0};
    pool[0].vel = (Vector3){0, -5.0f, 0}; pool[0].size = 0.3f; pool[0].life = 2.0f;
    ParticlesUpdate(pool, DEBRIS_MAX, 0.3f);        /* moves it below 0 -> bounce */
    assert(pool[0].pos.y == 0.0f);
    assert(pool[0].vel.y > 0.0f);                   /* bounced upward */

    /* expires when life runs out */
    memset(pool, 0, sizeof pool);
    pool[0].active = true; pool[0].size = 0.3f; pool[0].life = 0.05f;
    ParticlesUpdate(pool, DEBRIS_MAX, 0.1f);
    assert(!pool[0].active);

    printf("ok test_particle\n");
}
