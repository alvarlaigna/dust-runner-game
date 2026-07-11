/*
 * Dust Runner - parts.h
 * Modular "rig": mergeable parts and their effects on the vehicle.
 * Pure free functions over plain data (no raylib calls) so they unit-test.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

/* Part metadata (name, Lv3 label, category). */
const PartDef *PartGetDef(PartType t);

/* Fill `count` random part types (duplicates allowed) into `out`. */
void PartsRollChoices(PartType *out, int count);

/* Draft a fresh Lv1 part of type t into the rig. Ordering:
 *   1. completes a triple -> merge now, even on a full board (no scrap)
 *   2. else an empty slot  -> place there
 *   3. else (full, no merge)-> auto-scrap the weakest slot (pair-protecting)
 * Then cascade. Returns merges performed; writes last upgraded slot (or -1). */
int  PartsDraft(HexSlot slots[RIG_SLOTS], PartType t, int *outFlashSlot);

/* Fuse triples until stable (cascades L1->L2->L3). Returns merges; writes
 * last upgraded slot index into *outFlashSlot if non-NULL. */
int  PartsMergePass(HexSlot slots[RIG_SLOTS], int *outFlashSlot);

/* Reset the upgradeable stats to base, then apply every filled slot.
 * Never touches hp/hpMax or the runtime cooldowns. */
void VehicleRecomputeStats(Vehicle *v);

/* Apply armour, clamp, then the Repulsor Shield, then subtract HP. */
void VehicleTakeDamage(Vehicle *v, int rawDmg);

/* Kinetic Bumper: damage enemies the moving vehicle touches. Call per frame. */
void PartsRamPass(Vehicle *v, Enemy *enemies, int maxCount, float dt);
