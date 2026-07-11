/*
 * Dust Runner - parts.c
 * Modular "rig": mergeable parts and their effects on the vehicle.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "parts.h"
#include <stdlib.h>

static const PartDef _parts[PART_TYPE_COUNT] = {
    { "Autocannon",     "Chain Lightning",  UPCAT_WEAPON  },
    { "Payload",        "Explosive",        UPCAT_WEAPON  },
    { "Frost Splitter", "Freeze",           UPCAT_WEAPON  },
    { "Plating",        "Slow Aura",        UPCAT_CHASSIS },
    { "Nitro",          "Kinetic Bumper",   UPCAT_CHASSIS },
    { "Scrap Magnet",   "Repulsor Shield",  UPCAT_UTILITY },
};

const PartDef *PartGetDef(PartType t) {
    if (t < 0 || t >= PART_TYPE_COUNT) return &_parts[0];
    return &_parts[t];
}

void PartsRollChoices(PartType *out, int count) {
    for (int i = 0; i < count; i++)
        out[i] = (PartType)(rand() % PART_TYPE_COUNT);
}

int PartsMergePass(HexSlot slots[RIG_SLOTS], int *outFlashSlot) {
    int merges = 0;
    bool changed = true;
    while (changed) {
        changed = false;
        for (int type = 0; type < PART_TYPE_COUNT && !changed; type++) {
            for (int lvl = 1; lvl < PART_MAX_LEVEL && !changed; lvl++) {
                int idx[RIG_SLOTS], n = 0;
                for (int i = 0; i < RIG_SLOTS; i++)
                    if (slots[i].filled && slots[i].type == (PartType)type && slots[i].level == lvl)
                        idx[n++] = i;
                if (n >= 3) {
                    slots[idx[0]].level = lvl + 1;
                    slots[idx[1]].filled = false;
                    slots[idx[2]].filled = false;
                    if (outFlashSlot) *outFlashSlot = idx[0];
                    merges++;
                    changed = true;   /* restart scan so cascades resolve */
                }
            }
        }
    }
    return merges;
}

/* Count filled slots sharing slot i's (type, level). */
static int PartsCountSame(const HexSlot slots[RIG_SLOTS], int i) {
    int n = 0;
    for (int j = 0; j < RIG_SLOTS; j++)
        if (slots[j].filled && slots[j].type == slots[i].type && slots[j].level == slots[i].level)
            n++;
    return n;
}

/* Weakest slot to scrap on a full board: lowest level, then prefer a singleton
 * over a member of a pair, then lowest index. Assumes at least one filled slot. */
static int PartsWeakestSlot(const HexSlot slots[RIG_SLOTS]) {
    int best = -1;
    for (int i = 0; i < RIG_SLOTS; i++) {
        if (!slots[i].filled) continue;
        if (best < 0) { best = i; continue; }
        if (slots[i].level != slots[best].level) {
            if (slots[i].level < slots[best].level) best = i;
            continue;
        }
        int ci = PartsCountSame(slots, i), cb = PartsCountSame(slots, best);
        if (ci < cb) best = i;   /* prefer singleton; equal -> keep lower index */
    }
    return best;
}

int PartsDraft(HexSlot slots[RIG_SLOTS], PartType t, int *outFlashSlot) {
    if (outFlashSlot) *outFlashSlot = -1;

    int empty = -1, l1 = 0;
    for (int i = 0; i < RIG_SLOTS; i++) {
        if (!slots[i].filled) { if (empty < 0) empty = i; continue; }
        if (slots[i].type == t && slots[i].level == 1) l1++;
    }

    if (empty >= 0) {
        slots[empty].type = t; slots[empty].level = 1; slots[empty].filled = true;
        return PartsMergePass(slots, outFlashSlot);
    }

    /* Board full. */
    if (l1 >= 2) {
        /* Floating merge: fuse the pick with two existing L1s (temporary 7th slot). */
        int a = -1, b = -1;
        for (int i = 0; i < RIG_SLOTS; i++)
            if (slots[i].filled && slots[i].type == t && slots[i].level == 1) {
                if (a < 0) a = i; else { b = i; break; }
            }
        slots[a].level = 2;
        slots[b].filled = false;
        if (outFlashSlot) *outFlashSlot = a;
        return 1 + PartsMergePass(slots, outFlashSlot);
    }

    /* Full, no merge: scrap the weakest slot (pair-protecting), then place. */
    int w = PartsWeakestSlot(slots);
    slots[w].type = t; slots[w].level = 1; slots[w].filled = true;
    return PartsMergePass(slots, outFlashSlot);
}

/* Apply one installed part's contribution on top of the current (reset) stats. */
static void PartApply(Vehicle *v, PartType t, int level) {
    switch (t) {
    case PART_AUTOCANNON:
        v->fireRate *= (level == 1 ? CANNON_FR_L1 : level == 2 ? CANNON_FR_L2 : CANNON_FR_L3);
        if (level >= PART_MAX_LEVEL) v->hasChainLightning = true;
        break;
    case PART_PAYLOAD:
        v->damage += (level == 1 ? PAYLOAD_L1 : level == 2 ? PAYLOAD_L2 : PAYLOAD_L3);
        if (level >= PART_MAX_LEVEL) v->hasExplosive = true;
        break;
    case PART_FROST:
        v->projCount += level;   /* +1 / +2 / +3 */
        if (level >= PART_MAX_LEVEL) v->hasFreeze = true;
        break;
    case PART_PLATING:
        v->armor += (level == 1 ? PLATING_L1 : level == 2 ? PLATING_L2 : PLATING_L3);
        if (level >= PART_MAX_LEVEL) {
            if (PLATING_AURA_R  > v->slowAuraRadius) v->slowAuraRadius = PLATING_AURA_R;
            if (PLATING_AURA_MOD < v->slowAuraMod)   v->slowAuraMod    = PLATING_AURA_MOD;
        }
        break;
    case PART_NITRO:
        v->speed *= (level == 1 ? NITRO_L1 : level == 2 ? NITRO_L2 : NITRO_L3);
        if (level >= PART_MAX_LEVEL) v->ramDamage = (float)RAM_DMG_L3;   /* enables Kinetic Bumper */
        break;
    case PART_MAGNET: {
        float r = (level == 1 ? MAGNET_L1 : level == 2 ? MAGNET_L2 : MAGNET_L3);
        v->hasMagnet = true;
        if (r > v->magnetRadius) v->magnetRadius = r;
        if (level >= PART_MAX_LEVEL) v->hasShield = true;
        break;
    }
    default: break;
    }
}

void VehicleRecomputeStats(Vehicle *v) {
    /* reset upgradeable stats to base (hp/hpMax and cooldowns are left alone) */
    v->speed     = VEHICLE_SPEED_BASE;
    v->fireRate  = TURRET_FIRE_RATE;
    v->damage    = PROJ_DAMAGE_BASE;
    v->projCount = 1;
    v->armor     = 0;
    v->ramDamage = 0.0f;
    v->hasChainLightning = false;
    v->hasFreeze         = false;
    v->hasExplosive      = false;
    v->slowAuraRadius = 0.0f;
    v->slowAuraMod    = 1.0f;
    v->hasMagnet   = false;
    v->magnetRadius = 0.0f;
    v->hasShield   = false;

    for (int i = 0; i < RIG_SLOTS; i++)
        if (v->slots[i].filled) PartApply(v, v->slots[i].type, v->slots[i].level);
}

void VehicleTakeDamage(Vehicle *v, int rawDmg) {
    int dmg = rawDmg - v->armor;
    if (dmg < 1) dmg = 1;
    if (v->hasShield && v->shieldCooldown <= 0.0f) {
        v->shieldCooldown = SHIELD_COOLDOWN;   /* absorb one hit */
        return;
    }
    v->hp -= dmg;
}

void PartsRamPass(Vehicle *v, Enemy *enemies, int maxCount, float dt) {
    if (v->ramCooldown > 0.0f) v->ramCooldown -= dt;
    if (v->ramDamage <= 0.0f) return;              /* no Kinetic Bumper */
    if (v->curSpeed < RAM_MIN_SPEED) return;       /* must be moving */
    if (v->ramCooldown > 0.0f) return;             /* still cooling down */

    float frac = (v->speed > 0.0f) ? v->curSpeed / v->speed : 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    int dmg = (int)(v->ramDamage * (RAM_SPEED_MIN_SCALE + (1.0f - RAM_SPEED_MIN_SCALE) * frac));

    bool hit = false;
    float r2 = RAM_RADIUS * RAM_RADIUS;
    for (int i = 0; i < maxCount; i++) {
        if (!enemies[i].active) continue;
        float dx = enemies[i].pos.x - v->pos.x;
        float dz = enemies[i].pos.z - v->pos.z;
        if (dx * dx + dz * dz < r2) { enemies[i].hp -= dmg; hit = true; }
    }
    if (hit) v->ramCooldown = RAM_COOLDOWN;
}
