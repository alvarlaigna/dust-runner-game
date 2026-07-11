/*
 * Dust Runner - wave.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "wave.h"
#include "enemy.h"
#include <math.h>
#include <stdlib.h>

/* Simple LCG for deterministic-ish spawning */
static unsigned int _wrand = 12345;
static float WRand(void) {
    _wrand = _wrand * 1664525u + 1013904223u;
    return (float)(_wrand & 0xFFFF) / 65535.0f;
}

/* Spawn on the map edge, random side */
static Vector3 EdgeSpawnPos(const Vehicle *v) {
    float margin = 8.0f;
    float mapSize = TERRAIN_WORLD;
    int side = (int)(WRand() * 4.0f) % 4;
    float x, z;
    switch (side) {
        case 0: x = margin + WRand() * (mapSize - 2*margin); z = margin; break;
        case 1: x = margin + WRand() * (mapSize - 2*margin); z = mapSize - margin; break;
        case 2: x = margin;            z = margin + WRand() * (mapSize - 2*margin); break;
        default:x = mapSize - margin;  z = margin + WRand() * (mapSize - 2*margin); break;
    }
    (void)v;
    return (Vector3){ x, 0.0f, z };
}

void WaveInit(WaveState *w) {
    w->number        = 0;
    w->enemiesLeft   = 0;
    w->enemiesTotal  = 0;
    w->spawnTimer    = 0.0f;
    w->spawnInterval = 1.5f;
    w->respiteTimer  = 0.0f;
    w->bossWave      = false;
}

void WaveStartNext(WaveState *w) {
    w->number++;
    _wrand += (unsigned int)w->number * 31337;

    w->bossWave = (w->number % 5 == 0);

    /* Scale enemy count: 6 + 3*wave, capped at 40 */
    int count = 6 + 3 * (w->number - 1);
    if (count > 40) count = 40;
    if (w->bossWave) count = 1; /* boss wave: just the boss */

    w->enemiesTotal  = count;
    w->enemiesLeft   = count;
    w->spawnTimer    = 0.0f;
    w->spawnInterval = w->bossWave ? 0.5f :
                       (w->number < 5 ? 1.5f : 0.8f);
}

void WaveUpdate(WaveState *w, Enemy *pool, int poolMax, const Vehicle *v, float dt) {
    if (w->enemiesLeft <= 0) return;

    w->spawnTimer -= dt;
    if (w->spawnTimer > 0.0f) return;
    w->spawnTimer = w->spawnInterval;

    /* Choose enemy type based on wave number */
    EnemyType type;
    if (w->bossWave) {
        type = ENEMY_BOSS;
    } else {
        float r = WRand();
        if (w->number < 3) {
            type = ENEMY_RAIDER;
        } else if (w->number < 6) {
            type = r < 0.6f ? ENEMY_RAIDER : ENEMY_SHOOTER;
        } else if (w->number < 10) {
            if      (r < 0.4f) type = ENEMY_RAIDER;
            else if (r < 0.65f) type = ENEMY_SHOOTER;
            else if (r < 0.80f) type = ENEMY_HEAVY;
            else                type = ENEMY_SWARM;
        } else {
            if      (r < 0.25f) type = ENEMY_RAIDER;
            else if (r < 0.45f) type = ENEMY_SHOOTER;
            else if (r < 0.65f) type = ENEMY_HEAVY;
            else                type = ENEMY_SWARM;
        }
    }

    Vector3 spawnPos = EdgeSpawnPos(v);
    EnemySpawn(pool, poolMax, type, spawnPos, w->number);
    w->enemiesLeft--;
}

bool WaveIsComplete(const WaveState *w, const Enemy *pool, int poolMax) {
    if (w->enemiesLeft > 0) return false;
    return EnemiesAliveCount(pool, poolMax) == 0;
}
