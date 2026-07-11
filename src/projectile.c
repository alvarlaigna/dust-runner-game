/*
 * Dust Runner - projectile.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "projectile.h"
#include "enemy.h"
#include <math.h>
#include <string.h>

void ProjectileFire(Projectile *pool, int maxCount, Vector3 origin,
                    Vector3 dir, const Vehicle *v)
{
    /* Multi-shot: spread projCount projectiles in a small fan */
    float spreadStep = (v->projCount > 1) ? 8.0f / (v->projCount - 1) : 0.0f;
    float startAngle = (v->projCount > 1) ? -4.0f : 0.0f;

    int fired = 0;
    for (int slot = 0; slot < maxCount && fired < v->projCount; slot++) {
        if (pool[slot].active) continue;

        float angle = (startAngle + spreadStep * fired) * DEG2RAD;
        /* Rotate dir around Y */
        Vector3 d = {
            dir.x * cosf(angle) - dir.z * sinf(angle),
            0.0f,
            dir.x * sinf(angle) + dir.z * cosf(angle)
        };
        d = Vector3Normalize(d);

        pool[slot].active    = true;
        pool[slot].pos       = (Vector3){ origin.x, 1.2f, origin.z };
        pool[slot].vel       = Vector3Scale(d, PROJ_SPEED);
        pool[slot].damage    = v->damage;
        pool[slot].lifetime  = 2.0f;
        pool[slot].explosive = v->hasExplosive;
        pool[slot].freeze    = v->hasFreeze;
        pool[slot].chain     = v->hasChainLightning;
        fired++;
    }
}

static void ExplodeAt(Vector3 pos, Enemy *enemies, int enemyMax, int damage) {
    float r2 = 5.0f * 5.0f;
    for (int i = 0; i < enemyMax; i++) {
        if (!enemies[i].active) continue;
        float dx = enemies[i].pos.x - pos.x;
        float dz = enemies[i].pos.z - pos.z;
        if (dx*dx + dz*dz < r2) enemies[i].hp -= damage / 2;
    }
}

static void ChainLightning(Vector3 pos, Enemy *hit, Enemy *enemies, int enemyMax, int damage) {
    /* Jump to up to 2 additional nearby enemies */
    int jumps = 0;
    Enemy *last = hit;
    for (int j = 0; j < 2 && jumps < 2; j++) {
        float bestD = 8.0f * 8.0f;
        Enemy *next = NULL;
        for (int i = 0; i < enemyMax; i++) {
            if (!enemies[i].active || &enemies[i] == last || &enemies[i] == hit) continue;
            float dx = enemies[i].pos.x - last->pos.x;
            float dz = enemies[i].pos.z - last->pos.z;
            float d2 = dx*dx + dz*dz;
            if (d2 < bestD) { bestD = d2; next = &enemies[i]; }
        }
        if (next) { next->hp -= damage / 2; last = next; jumps++; }
    }
    (void)pos;
}

void ProjectilesUpdate(Projectile *pool, int maxCount, Enemy *enemies,
                       int enemyMax, float dt)
{
    for (int i = 0; i < maxCount; i++) {
        if (!pool[i].active) continue;

        pool[i].pos = Vector3Add(pool[i].pos, Vector3Scale(pool[i].vel, dt));
        pool[i].lifetime -= dt;

        if (pool[i].lifetime <= 0.0f) { pool[i].active = false; continue; }

        /* Collision with enemies */
        for (int j = 0; j < enemyMax; j++) {
            if (!enemies[j].active) continue;
            static const float _ESIZE[ENEMY_TYPE_COUNT] = { 1.0f, 0.9f, 1.6f, 0.7f, 2.8f };
            float sz = _ESIZE[enemies[j].type];
            float dx = pool[i].pos.x - enemies[j].pos.x;
            float dz = pool[i].pos.z - enemies[j].pos.z;
            float d2 = dx*dx + dz*dz;
            float hitR = sz * 0.5f + 0.3f;
            if (d2 < hitR * hitR) {
                enemies[j].hp -= pool[i].damage;

                if (pool[i].freeze)    enemies[j].frozenTimer = 2.0f;
                if (pool[i].explosive) ExplodeAt(pool[i].pos, enemies, enemyMax, pool[i].damage);
                if (pool[i].chain)     ChainLightning(pool[i].pos, &enemies[j], enemies, enemyMax, pool[i].damage);

                pool[i].active = false;
                break;
            }
        }
    }
}

void ProjectilesDraw(const Projectile *pool, int maxCount) {
    for (int i = 0; i < maxCount; i++) {
        if (!pool[i].active) continue;
        Color col = pool[i].freeze    ? (Color){100, 200, 255, 255} :
                    pool[i].explosive ? (Color){255, 140, 30,  255} :
                    pool[i].chain     ? (Color){180, 100, 255, 255} :
                                        (Color){255, 240, 80,  255};
        DrawSphere(pool[i].pos, 0.18f, col);
    }
}
