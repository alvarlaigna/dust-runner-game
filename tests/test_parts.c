/*
 * Dust Runner - tests/test_parts.c
 * Pure unit tests for the rig logic. No window; links parts.c only.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "parts.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

void run_particle_tests(void);

static void clearSlots(HexSlot s[RIG_SLOTS]) {
    for (int i = 0; i < RIG_SLOTS; i++) { s[i].filled = false; s[i].level = 0; s[i].type = 0; }
}
static void setSlot(HexSlot s[RIG_SLOTS], int i, PartType t, int lv) {
    s[i].filled = true; s[i].type = t; s[i].level = lv;
}
static int countFilled(HexSlot s[RIG_SLOTS]) {
    int n = 0; for (int i = 0; i < RIG_SLOTS; i++) if (s[i].filled) n++; return n;
}
static int countTL(HexSlot s[RIG_SLOTS], PartType t, int lv) {
    int n = 0; for (int i = 0; i < RIG_SLOTS; i++) if (s[i].filled && s[i].type == t && s[i].level == lv) n++; return n;
}

static void test_merge_triple(void) {
    HexSlot s[RIG_SLOTS]; clearSlots(s);
    setSlot(s, 0, PART_PAYLOAD, 1); setSlot(s, 1, PART_PAYLOAD, 1); setSlot(s, 2, PART_PAYLOAD, 1);
    int flash = -1;
    int m = PartsMergePass(s, &flash);
    assert(m == 1);
    assert(countTL(s, PART_PAYLOAD, 2) == 1);
    assert(countFilled(s) == 1);
    assert(flash >= 0 && s[flash].filled && s[flash].level == 2);
    printf("ok test_merge_triple\n");
}

static void test_merge_cascade(void) {
    HexSlot s[RIG_SLOTS]; clearSlots(s);
    /* two L2 + three L1 of one type: L1x3 -> L2, then L2x3 -> L3, in one pass */
    setSlot(s, 0, PART_NITRO, 2); setSlot(s, 1, PART_NITRO, 2);
    setSlot(s, 2, PART_NITRO, 1); setSlot(s, 3, PART_NITRO, 1); setSlot(s, 4, PART_NITRO, 1);
    int flash = -1;
    int m = PartsMergePass(s, &flash);
    assert(m == 2);
    assert(countTL(s, PART_NITRO, 3) == 1);
    assert(countFilled(s) == 1);
    printf("ok test_merge_cascade\n");
}

static void test_draft_place(void) {
    HexSlot s[RIG_SLOTS]; clearSlots(s);
    int flash = -1;
    int m = PartsDraft(s, PART_PLATING, &flash);
    assert(m == 0);
    assert(countFilled(s) == 1);
    assert(countTL(s, PART_PLATING, 1) == 1);
    printf("ok test_draft_place\n");
}

static void test_draft_floating(void) {
    HexSlot s[RIG_SLOTS]; clearSlots(s);
    /* full board: two Payload L1 + four singletons; drafting Payload completes a triple */
    setSlot(s, 0, PART_PAYLOAD, 1); setSlot(s, 1, PART_PAYLOAD, 1);
    setSlot(s, 2, PART_NITRO, 1);   setSlot(s, 3, PART_PLATING, 1);
    setSlot(s, 4, PART_MAGNET, 1);  setSlot(s, 5, PART_FROST, 1);
    assert(countFilled(s) == 6);
    int flash = -1;
    int m = PartsDraft(s, PART_PAYLOAD, &flash);
    assert(m >= 1);
    assert(countTL(s, PART_PAYLOAD, 2) == 1);
    assert(countTL(s, PART_PAYLOAD, 1) == 0);
    assert(countFilled(s) == 5);   /* merge freed a slot; nothing scrapped */
    printf("ok test_draft_floating\n");
}

static void test_draft_scrap_protects_pair(void) {
    HexSlot s[RIG_SLOTS]; clearSlots(s);
    /* full board: a Nitro L1 PAIR + four singletons; draft a new non-merging type */
    setSlot(s, 0, PART_NITRO, 1);   setSlot(s, 1, PART_NITRO, 1);
    setSlot(s, 2, PART_PLATING, 1); setSlot(s, 3, PART_MAGNET, 1);
    setSlot(s, 4, PART_PAYLOAD, 1); setSlot(s, 5, PART_AUTOCANNON, 1);
    int flash = -1;
    int m = PartsDraft(s, PART_FROST, &flash);
    assert(m == 0);
    assert(countTL(s, PART_NITRO, 1) == 2);   /* pair survived */
    assert(countTL(s, PART_FROST, 1) == 1);   /* new part placed */
    assert(countFilled(s) == 6);
    printf("ok test_draft_scrap_protects_pair\n");
}

static void test_recompute(void) {
    Vehicle v; memset(&v, 0, sizeof v);

    VehicleRecomputeStats(&v);   /* empty rig -> base */
    assert(fabsf(v.fireRate - TURRET_FIRE_RATE) < 0.001f);
    assert(v.damage == PROJ_DAMAGE_BASE);
    assert(v.projCount == 1);
    assert(v.armor == 0);
    assert(!v.hasChainLightning && !v.hasExplosive && !v.hasFreeze);
    assert(v.ramDamage == 0.0f);

    /* Autocannon L3 -> chain + fireRate x1.95 */
    v.slots[0].filled = true; v.slots[0].type = PART_AUTOCANNON; v.slots[0].level = 3;
    VehicleRecomputeStats(&v);
    assert(v.hasChainLightning);
    assert(fabsf(v.fireRate - TURRET_FIRE_RATE * CANNON_FR_L3) < 0.001f);

    /* stacking: a stray Autocannon L1 multiplies again */
    v.slots[1].filled = true; v.slots[1].type = PART_AUTOCANNON; v.slots[1].level = 1;
    VehicleRecomputeStats(&v);
    assert(fabsf(v.fireRate - TURRET_FIRE_RATE * CANNON_FR_L3 * CANNON_FR_L1) < 0.001f);

    /* Magnet L3 -> shield + magnet; Nitro L3 -> ram enabled */
    memset(&v, 0, sizeof v);
    v.slots[0].filled = true; v.slots[0].type = PART_MAGNET; v.slots[0].level = 3;
    v.slots[1].filled = true; v.slots[1].type = PART_NITRO;  v.slots[1].level = 3;
    VehicleRecomputeStats(&v);
    assert(v.hasShield && v.hasMagnet);
    assert(v.ramDamage == (float)RAM_DMG_L3);

    /* additive stacking (Payload L1+L2) and MAX stacking (Magnet L3 then L1) */
    memset(&v, 0, sizeof v);
    v.slots[0].filled = true; v.slots[0].type = PART_PAYLOAD; v.slots[0].level = 1;
    v.slots[1].filled = true; v.slots[1].type = PART_PAYLOAD; v.slots[1].level = 2;
    v.slots[2].filled = true; v.slots[2].type = PART_MAGNET;  v.slots[2].level = 3;
    v.slots[3].filled = true; v.slots[3].type = PART_MAGNET;  v.slots[3].level = 1;
    VehicleRecomputeStats(&v);
    assert(v.damage == PROJ_DAMAGE_BASE + PAYLOAD_L1 + PAYLOAD_L2);  /* additive */
    assert(fabsf(v.magnetRadius - MAGNET_L3) < 0.001f);             /* MAX, not last-wins */
    assert(v.hasShield);

    printf("ok test_recompute\n");
}

static void test_take_damage(void) {
    Vehicle v; memset(&v, 0, sizeof v);
    v.hp = 100; v.armor = 5;
    VehicleTakeDamage(&v, 20); assert(v.hp == 85);          /* 20 - 5 */
    v.armor = 100;
    VehicleTakeDamage(&v, 5);  assert(v.hp == 84);          /* clamp to 1 */

    memset(&v, 0, sizeof v);
    v.hp = 100; v.hasShield = true; v.shieldCooldown = 0.0f;
    VehicleTakeDamage(&v, 30); assert(v.hp == 100);         /* absorbed */
    assert(v.shieldCooldown > 0.0f);
    VehicleTakeDamage(&v, 30); assert(v.hp == 70);          /* on cooldown -> taken */
    printf("ok test_take_damage\n");
}

static void test_ram(void) {
    Vehicle v; memset(&v, 0, sizeof v);
    v.pos = (Vector3){ 0, 0, 0 };
    v.speed = 18.0f; v.curSpeed = 18.0f; v.ramDamage = (float)RAM_DMG_L3; v.ramCooldown = 0.0f;

    Enemy e[2]; memset(e, 0, sizeof e);
    e[0].active = true; e[0].pos = (Vector3){ 1, 0, 1 };   e[0].hp = 100;   /* in radius */
    e[1].active = true; e[1].pos = (Vector3){ 50, 0, 50 }; e[1].hp = 100;   /* far */

    PartsRamPass(&v, e, 2, 0.016f);
    assert(e[0].hp < 100);
    assert(e[1].hp == 100);
    assert(v.ramCooldown > 0.0f);
    int after = e[0].hp;
    PartsRamPass(&v, e, 2, 0.016f);   /* still on cooldown */
    assert(e[0].hp == after);

    /* no Nitro L3 -> ramDamage 0 -> no ram */
    memset(&v, 0, sizeof v);
    v.speed = 18.0f; v.curSpeed = 18.0f; v.ramDamage = 0.0f;
    e[0].hp = 100;
    PartsRamPass(&v, e, 2, 0.016f);
    assert(e[0].hp == 100);

    /* partial speed -> partial ram damage via RAM_SPEED_MIN_SCALE (frac 0.5) */
    memset(&v, 0, sizeof v);
    v.speed = 18.0f; v.curSpeed = 9.0f;
    v.ramDamage = (float)RAM_DMG_L3; v.ramCooldown = 0.0f;
    e[0].active = true; e[0].pos = (Vector3){ 0.5f, 0, 0.5f }; e[0].hp = 100;
    PartsRamPass(&v, e, 2, 0.016f);
    {
        int expect = (int)(RAM_DMG_L3 * (RAM_SPEED_MIN_SCALE + (1.0f - RAM_SPEED_MIN_SCALE) * 0.5f));
        assert(e[0].hp == 100 - expect);
    }
    printf("ok test_ram\n");
}

static void test_roll(void) {
    bool seen[PART_TYPE_COUNT] = { false };
    int distinct = 0;
    for (int trial = 0; trial < 50; trial++) {
        PartType c[UPGRADE_CHOICES];
        PartsRollChoices(c, UPGRADE_CHOICES);
        for (int i = 0; i < UPGRADE_CHOICES; i++) {
            assert(c[i] >= 0 && c[i] < PART_TYPE_COUNT);   /* in range */
            if (!seen[c[i]]) { seen[c[i]] = true; distinct++; }
        }
    }
    /* a constant or out-of-range impl would fail one of these assertions */
    assert(distinct > 1);
    printf("ok test_roll\n");
}

int main(void) {
    printf("running parts tests...\n");
    test_merge_triple();
    test_merge_cascade();
    test_draft_place();
    test_draft_floating();
    test_draft_scrap_protects_pair();
    test_recompute();
    test_take_damage();
    test_ram();
    test_roll();
    run_particle_tests();
    printf("ALL PARTS TESTS PASSED\n");
    return 0;
}
