/*
 * Dust Runner - upgrade.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "upgrade.h"
#include <stdlib.h>
#include <time.h>

static UpgradeDef _pool[] = {
    /* Weapon (id 0–9) */
    { "Rapid Fire",       "+30% fire rate",                UPCAT_WEAPON,  0  },
    { "Heavy Rounds",     "+10 damage per shot",           UPCAT_WEAPON,  1  },
    { "Twin Barrels",     "Fire one extra projectile",     UPCAT_WEAPON,  2  },
    { "Cryo Rounds",      "Shots freeze enemies 2s",       UPCAT_WEAPON,  3  },
    { "Explosive Tips",   "Shots explode on impact",       UPCAT_WEAPON,  4  },
    { "Chain Lightning",  "Shots arc to 2 nearby foes",   UPCAT_WEAPON,  5  },
    { "Incendiary",       "+5 damage, burn DoT",           UPCAT_WEAPON,  6  },
    { "Armour Pierce",    "Ignore 5 enemy armour",         UPCAT_WEAPON,  7  },
    { "Overcharge",       "+50% projectile speed",         UPCAT_WEAPON,  8  },
    { "Scatter Shot",     "+2 projectiles, -10% damage",   UPCAT_WEAPON,  9  },

    /* Chassis (id 10–19) */
    { "Nitro Boost",      "+20% vehicle speed",            UPCAT_CHASSIS, 10 },
    { "Plating",          "+5 flat armour",                UPCAT_CHASSIS, 11 },
    { "Ram Spikes",       "+50% ram damage",               UPCAT_CHASSIS, 12 },
    { "Reinforced Hull",  "+30 max HP, full heal",         UPCAT_CHASSIS, 13 },
    { "All-Terrain",      "Dunes no longer slow you",      UPCAT_CHASSIS, 14 },
    { "HP Regen",         "Regen 2 HP/s passively",        UPCAT_CHASSIS, 15 },
    { "Reactive Armour",  "Reflect 20% melee damage",      UPCAT_CHASSIS, 16 },
    { "Turbo Treads",     "Arrive steering 40% faster",    UPCAT_CHASSIS, 17 },
    { "Gyro Stabiliser",  "Turret rotates 50% faster",     UPCAT_CHASSIS, 18 },
    { "Salvage Kit",      "+50% scrap from pickups",       UPCAT_CHASSIS, 19 },

    /* Utility (id 20–29) */
    { "Slow Aura",        "Enemies in 8u radius slowed",   UPCAT_UTILITY, 20 },
    { "Scrap Magnet",     "Auto-collect scrap in 12u",     UPCAT_UTILITY, 21 },
    { "Energy Shield",    "Absorb one hit every 10s",      UPCAT_UTILITY, 22 },
    { "Decoy Drone",      "Drone distracts enemies 8s",    UPCAT_UTILITY, 23 },
    { "Radar Pulse",      "Reveal enemy positions 5s",     UPCAT_UTILITY, 24 },
    { "EMP Burst",        "Stun all enemies 3s on use",    UPCAT_UTILITY, 25 },
    { "Repair Nanobots",  "Heal 25 HP immediately",        UPCAT_UTILITY, 26 },
    { "Overclock",        "Double fire rate for 5s",       UPCAT_UTILITY, 27 },
    { "Minefield",        "Drop 3 mines on right-click",   UPCAT_UTILITY, 28 },
    { "Airstrike Beacon", "Call strike on target area",    UPCAT_UTILITY, 29 },
};

#define POOL_COUNT (sizeof(_pool) / sizeof(_pool[0]))

void UpgradePoolInit(void) {
    srand((unsigned int)time(NULL));
}

void UpgradePickChoices(UpgradeDef **choices, int count) {
    /* Fisher-Yates partial shuffle to pick `count` unique entries */
    int indices[POOL_COUNT];
    for (int i = 0; i < (int)POOL_COUNT; i++) indices[i] = i;
    for (int i = 0; i < count; i++) {
        int j = i + rand() % (POOL_COUNT - i);
        int tmp = indices[i]; indices[i] = indices[j]; indices[j] = tmp;
        choices[i] = &_pool[indices[i]];
    }
}

const UpgradeDef *UpgradeGetById(int id) {
    for (int i = 0; i < (int)POOL_COUNT; i++) {
        if (_pool[i].id == id) return &_pool[i];
    }
    return NULL;
}
