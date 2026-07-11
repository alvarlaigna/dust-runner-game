/*
 * Dust Runner - enemy.c
 * Billboard enemies with per-type AI.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "enemy.h"
#include "projectile.h"
#include <math.h>
#include <string.h>

/* Per-type base stats */
static const int   BASE_HP[ENEMY_TYPE_COUNT]    = { 40, 30, 120, 15, 500 };
static const float BASE_SPEED[ENEMY_TYPE_COUNT] = { 6.0f, 4.5f, 3.0f, 9.0f, 2.5f };
static const Color ENEMY_COL[ENEMY_TYPE_COUNT]  = {
    {220, 60,  40,  255},  /* raider  - red     */
    {60,  140, 220, 255},  /* shooter - blue    */
    {160, 60,  160, 255},  /* heavy   - purple  */
    {240, 200, 40,  255},  /* swarm   - yellow  */
    {220, 120, 20,  255},  /* boss    - orange  */
};
static const float ENEMY_SIZE[ENEMY_TYPE_COUNT] = { 1.0f, 0.9f, 1.6f, 0.7f, 2.8f };

void EnemySpawn(Enemy *pool, int maxCount, EnemyType type, Vector3 pos, int waveNum) {
    for (int i = 0; i < maxCount; i++) {
        if (!pool[i].active) {
            pool[i].active        = true;
            pool[i].pos           = pos;
            pool[i].type          = type;
            float scale           = 1.0f + (waveNum - 1) * 0.08f;
            pool[i].hpMax         = (int)(BASE_HP[type] * scale);
            pool[i].hp            = pool[i].hpMax;
            pool[i].speed         = BASE_SPEED[type] * (1.0f + (waveNum - 1) * 0.04f);
            pool[i].attackCooldown = 0.0f;
            pool[i].frozenTimer   = 0.0f;
            pool[i].angle         = 0.0f;
            return;
        }
    }
}

/* Per-type AI update */
static void UpdateRaider(Enemy *e, Vehicle *v, float dt) {
    /* Rush directly at vehicle */
    Vector3 d = Vector3Subtract(v->pos, e->pos);
    d.y = 0;
    float dist = Vector3Length(d);
    if (dist < 0.1f) return;
    Vector3 dir = Vector3Normalize(d);
    e->pos = Vector3Add(e->pos, Vector3Scale(dir, e->speed * dt));
    e->angle = -(atan2f(dir.z, dir.x) * RAD2DEG) + 90.0f;

    /* Melee damage on contact */
    if (dist < 2.0f) {
        e->attackCooldown -= dt;
        if (e->attackCooldown <= 0.0f) {
            int dmg = (int)(8.0f * (e->type == ENEMY_BOSS ? 3.0f : 1.0f));
            dmg = dmg - v->armor;
            if (dmg < 1) dmg = 1;
            v->hp -= dmg;
            e->attackCooldown = 1.2f;
        }
    }
}

static void UpdateShooter(Enemy *e, Vehicle *v, Projectile *projPool, int projMax, float dt) {
    /* Keep preferred distance ~14 units, shoot if in range */
    Vector3 d = Vector3Subtract(v->pos, e->pos);
    d.y = 0;
    float dist = Vector3Length(d);
    Vector3 dir = Vector3Normalize(d);

    float preferred = 14.0f;
    if (dist > preferred + 2.0f) {
        e->pos = Vector3Add(e->pos, Vector3Scale(dir, e->speed * dt));
    } else if (dist < preferred - 2.0f) {
        e->pos = Vector3Subtract(e->pos, Vector3Scale(dir, e->speed * 0.5f * dt));
    }
    e->angle = -(atan2f(dir.z, dir.x) * RAD2DEG) + 90.0f;

    e->attackCooldown -= dt;
    if (e->attackCooldown <= 0.0f && dist < 20.0f) {
        /* Fire a slow projectile at vehicle */
        for (int i = 0; i < projMax; i++) {
            if (!projPool[i].active) {
                projPool[i].active   = true;
                projPool[i].pos      = e->pos;
                projPool[i].vel      = Vector3Scale(dir, 12.0f);
                projPool[i].damage   = 8;
                projPool[i].lifetime = 3.0f;
                projPool[i].explosive = false;
                projPool[i].freeze   = false;
                projPool[i].chain    = false;
                break;
            }
        }
        e->attackCooldown = 2.0f;
    }
}

static void UpdateHeavy(Enemy *e, Vehicle *v, float dt) {
    /* Slow charge */
    Vector3 d = Vector3Subtract(v->pos, e->pos);
    d.y = 0;
    float dist = Vector3Length(d);
    if (dist < 0.1f) return;
    Vector3 dir = Vector3Normalize(d);
    e->pos = Vector3Add(e->pos, Vector3Scale(dir, e->speed * dt));
    e->angle = -(atan2f(dir.z, dir.x) * RAD2DEG) + 90.0f;

    if (dist < 2.5f) {
        e->attackCooldown -= dt;
        if (e->attackCooldown <= 0.0f) {
            int dmg = 20 - v->armor;
            if (dmg < 1) dmg = 1;
            v->hp -= dmg;
            e->attackCooldown = 2.0f;
        }
    }
}

static void UpdateSwarm(Enemy *e, Vehicle *v, float dt, int idx) {
    /* Fast zigzag approach */
    Vector3 d = Vector3Subtract(v->pos, e->pos);
    d.y = 0;
    float dist = Vector3Length(d);
    if (dist < 0.1f) return;
    Vector3 dir = Vector3Normalize(d);

    /* Perpendicular wobble; phase per pool slot (idx is exact as a float). */
    float t = (float)GetTime() * 3.0f + (float)idx * 0.7f;
    Vector3 perp = { -dir.z, 0, dir.x };
    dir = Vector3Add(dir, Vector3Scale(perp, sinf(t) * 0.4f));
    dir = Vector3Normalize(dir);

    e->pos = Vector3Add(e->pos, Vector3Scale(dir, e->speed * dt));
    e->angle = -(atan2f(dir.z, dir.x) * RAD2DEG) + 90.0f;

    if (dist < 1.5f) {
        e->attackCooldown -= dt;
        if (e->attackCooldown <= 0.0f) {
            int dmg = 5 - v->armor;
            if (dmg < 1) dmg = 1;
            v->hp -= dmg;
            e->attackCooldown = 0.8f;
        }
    }
}

void EnemiesUpdate(Enemy *pool, int maxCount, Vehicle *v, Projectile *projPool,
                   int projMax, const Terrain *t, float dt, int *killCount)
{
    (void)t; /* terrain avoidance: future enhancement */
    for (int i = 0; i < maxCount; i++) {
        if (!pool[i].active) continue;

        /* Frozen timer */
        if (pool[i].frozenTimer > 0.0f) {
            pool[i].frozenTimer -= dt;
            continue; /* skip movement/attack while frozen */
        }

        switch (pool[i].type) {
            case ENEMY_RAIDER:  UpdateRaider(&pool[i], v, dt); break;
            case ENEMY_SHOOTER: UpdateShooter(&pool[i], v, projPool, projMax, dt); break;
            case ENEMY_HEAVY:   UpdateHeavy(&pool[i], v, dt); break;
            case ENEMY_SWARM:   UpdateSwarm(&pool[i], v, dt, i); break;
            case ENEMY_BOSS:    UpdateRaider(&pool[i], v, dt); break; /* boss uses raider AI for now */
            default: break;
        }

        /* Slow aura from vehicle */
        if (v->slowAuraRadius > 0.0f) {
            float d = Vector3Distance(pool[i].pos, v->pos);
            if (d < v->slowAuraRadius) {
                pool[i].speed = BASE_SPEED[pool[i].type] * v->slowAuraMod;
            } else {
                pool[i].speed = BASE_SPEED[pool[i].type];
            }
        }

        /* Death check */
        if (pool[i].hp <= 0) {
            pool[i].active = false;
            (*killCount)++;
        }
    }
}

/* Billboard drawing */
void EnemiesDraw(const Enemy *pool, int maxCount, const Camera3D *cam) {
    for (int i = 0; i < maxCount; i++) {
        if (!pool[i].active) continue;
        const Enemy *e = &pool[i];
        float sz = ENEMY_SIZE[e->type];
        Color col = ENEMY_COL[e->type];

        /* Frozen tint */
        if (e->frozenTimer > 0.0f) col = (Color){120, 200, 255, 255};

        /* Billboard: always face camera */
        DrawBillboard(*cam,
            (Texture2D){0},   /* no texture - we use a coloured rectangle */
            e->pos, sz, col);

        /* Fallback: draw a coloured cube since we have no texture */
        DrawCube((Vector3){e->pos.x, sz * 0.5f, e->pos.z}, sz * 0.7f, sz, sz * 0.7f, col);

        /* HP bar */
        float hpFrac = (float)e->hp / (float)e->hpMax;
        Vector3 barPos = { e->pos.x, sz + 0.3f, e->pos.z };
        DrawCube(barPos, sz * 0.8f, 0.1f, 0.05f, (Color){50, 50, 50, 200});
        Vector3 fillPos = { e->pos.x - sz * 0.4f + sz * 0.4f * hpFrac,
                            sz + 0.3f, e->pos.z };
        DrawCube(fillPos, sz * 0.8f * hpFrac, 0.12f, 0.06f,
                 (Color){50, 200, 50, 255});
    }
}

int EnemiesAliveCount(const Enemy *pool, int maxCount) {
    int c = 0;
    for (int i = 0; i < maxCount; i++) if (pool[i].active) c++;
    return c;
}

Enemy *EnemyFindNearest(Enemy *pool, int maxCount, Vector3 origin, float maxRange) {
    Enemy *best = NULL;
    float bestDist = maxRange * maxRange;
    for (int i = 0; i < maxCount; i++) {
        if (!pool[i].active) continue;
        float dx = pool[i].pos.x - origin.x;
        float dz = pool[i].pos.z - origin.z;
        float d2 = dx*dx + dz*dz;
        if (d2 < bestDist) { bestDist = d2; best = &pool[i]; }
    }
    return best;
}
