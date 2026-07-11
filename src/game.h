/*
 * Dust Runner - game.h
 * Shared types, constants, and forward declarations.
 *
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 * All rights reserved.
 */
#pragma once
#include "raylib.h"
#include "raymath.h"
#include <stdbool.h>
#include <stdint.h>

/* Constants */
#define SCREEN_W        720
#define SCREEN_H        720
#define TARGET_FPS      60

/* Camera framing (retuned for 1:1 aspect - verify visually) */
#define CAM_HEIGHT      54.0f   /* eye height (world units) */
#define CAM_BACK        48.0f   /* distance behind the vehicle on +Z */
#define CAM_FOVY        60.0f   /* vertical FOV; wider than 16:9's 52 to keep map width visible */

/* Version / attribution (single source of truth) */
#define GAME_VERSION    "v0.1.0"
#define GAME_AUTHOR     "Alvar Laigna"
#define GAME_URL        "alvarlaigna.itch.io"
/* ASCII only: the default raylib font lacks glyphs above 126 (UTF-8 shows tofu). */
#define GAME_CREDIT     "(c) 2026 Alvar Laigna - alvarlaigna.itch.io"

/* Terrain */
#define TERRAIN_TILES   80          /* tiles per axis */
#define TILE_SIZE       4.0f        /* world units */
#define TERRAIN_WORLD   (TERRAIN_TILES * TILE_SIZE)

/* Vehicle */
#define VEHICLE_SPEED_BASE  18.0f   /* world units / s (max speed) */
#define VEHICLE_ARRIVE_DIST  0.6f   /* stop radius */
#define VEHICLE_HP_BASE     100
/* Truck-like handling (bicycle steering model) */
#define VEHICLE_WHEELBASE    4.5f   /* larger = wider turning radius */
#define VEHICLE_MAX_STEER   32.0f   /* max wheel angle, degrees */
#define VEHICLE_STEER_RATE 140.0f   /* how fast the wheel slews, deg/s */
#define VEHICLE_ACCEL       26.0f   /* units/s^2 */
#define VEHICLE_BRAKE       38.0f   /* units/s^2 */
#define VEHICLE_BOUND_MARGIN 6.0f   /* keep the vehicle this far inside the map edge */

/* Turret */
#define TURRET_RANGE        28.0f
#define TURRET_ROT_SPEED    220.0f  /* deg / s */
#define TURRET_FIRE_RATE    1.5f    /* shots / s */

/* Projectile */
#define PROJ_SPEED          40.0f
#define PROJ_DAMAGE_BASE    15
#define PROJ_MAX            256

/* Enemy */
#define ENEMY_MAX           256
#define ENEMY_SPEED_BASE    6.0f
#define ENEMY_HP_BASE       40

/* Wave */
#define WAVE_RESPITE_TIME   5.0f    /* seconds between waves */
#define UPGRADE_CHOICES     3

/* Upgrade card geometry (shared by hud.c draw + main.c hit-test; fits 3-up in 720) */
#define UPGRADE_CARD_W    200
#define UPGRADE_CARD_H    150
#define UPGRADE_CARD_GAP  18

/* Upgrades */
#define UPGRADE_POOL_SIZE   30

/* Terrain tile types */
typedef enum {
    TILE_HARDPAN = 0,   /* 130% speed, ancient road remnant */
    TILE_SAND,          /* 100% speed, normal */
    TILE_DUNE,          /*  60% speed, soft sand */
    TILE_RUIN,          /*   0% speed, impassable / cover */
    TILE_COUNT
} TileType;

static const float TILE_SPEED_MOD[TILE_COUNT] = { 1.30f, 1.00f, 0.60f, 0.0f };
static const Color TILE_COLOR[TILE_COUNT] = {
    { 180, 160, 110, 255 },   /* hardpan - pale ochre */
    { 220, 190, 130, 255 },   /* sand    - warm tan  */
    { 200, 175, 100, 255 },   /* dune    - deep sand */
    {  90,  80,  65, 255 },   /* ruin    - dark grey */
};

/* Game states */
typedef enum {
    STATE_PLAYING = 0,
    STATE_RESPITE,
    STATE_UPGRADE,
    STATE_GAMEOVER,
    STATE_COUNT
} GameState;

/* Top-level app screens (above the gameplay state machine) */
typedef enum { SCREEN_TITLE = 0, SCREEN_GAMEPLAY } AppScreen;

/* Upgrade categories */
typedef enum {
    UPCAT_WEAPON = 0,
    UPCAT_CHASSIS,
    UPCAT_UTILITY,
} UpgradeCategory;

typedef struct {
    const char     *name;
    const char     *desc;
    UpgradeCategory category;
    int             id;         /* unique index into upgrade pool */
} UpgradeDef;

/* Vehicle */
typedef struct {
    Vector3  pos;
    Vector3  target;            /* move-to destination */
    float    angle;             /* body yaw, degrees */
    float    turretAngle;       /* turret yaw, degrees */
    bool     moving;
    bool     autoAim;

    /* stats (modified by upgrades) */
    float    speed;             /* max speed */
    float    curSpeed;          /* instantaneous forward speed (momentum) */
    float    steerAngle;        /* current steering/wheel angle, degrees */
    int      hp;
    int      hpMax;
    int      armor;             /* flat damage reduction */
    float    ramDamage;

    /* weapon stats */
    float    fireRate;          /* shots / s */
    float    fireCooldown;
    int      damage;
    int      projCount;         /* multi-shot */
    bool     hasChainLightning;
    bool     hasFreeze;
    bool     hasExplosive;

    /* utility */
    float    slowAuraRadius;
    float    slowAuraMod;
    bool     hasShield;
    float    shieldCooldown;
    bool     hasMagnet;
    float    magnetRadius;
} Vehicle;

/* Projectile */
typedef struct {
    Vector3  pos;
    Vector3  vel;
    int      damage;
    float    lifetime;
    bool     active;
    bool     explosive;
    bool     freeze;
    bool     chain;
} Projectile;

/* Enemy types */
typedef enum {
    ENEMY_RAIDER = 0,   /* basic melee rusher */
    ENEMY_SHOOTER,      /* ranged, keeps distance */
    ENEMY_HEAVY,        /* slow, high HP */
    ENEMY_SWARM,        /* fast, low HP, many */
    ENEMY_BOSS,         /* boss: large, special attack */
    ENEMY_TYPE_COUNT
} EnemyType;

typedef struct {
    Vector3   pos;
    float     angle;
    EnemyType type;
    int       hp;
    int       hpMax;
    float     speed;
    float     attackCooldown;
    float     frozenTimer;
    bool      active;
} Enemy;

/* Wave state */
typedef struct {
    int   number;           /* current wave index (1-based) */
    int   enemiesLeft;
    int   enemiesTotal;
    float spawnTimer;
    float spawnInterval;
    float respiteTimer;
    bool  bossWave;
} WaveState;

/* Terrain */
typedef struct {
    TileType tiles[TERRAIN_TILES][TERRAIN_TILES];
    /* scrap piles (collectible meta-currency) */
    Vector3  scrapPos[64];
    bool     scrapActive[64];
    int      scrapCount;
} Terrain;

/* First-run tutorial (contextual teach-by-doing). */
typedef enum { TUT_MOVE = 0, TUT_FIRE, TUT_DONE } TutorialStep;

/* Full game context */
typedef struct {
    GameState   state;
    Vehicle     vehicle;
    Enemy       enemies[ENEMY_MAX];
    Projectile  projectiles[PROJ_MAX];
    Terrain     terrain;
    WaveState   wave;
    Camera3D    camera;

    /* upgrade selection */
    UpgradeDef *upgradeChoices[UPGRADE_CHOICES];
    int         upgradeHover;

    /* score / meta */
    int         score;
    int         scrapCollected;
    int         kills;
    double      runTime;

    /* manual fire override */
    bool        manualFireQueued;
    Vector3     manualFireTarget;

    /* pause menu */
    bool        paused;

    /* first-run tutorial */
    TutorialStep tutStep;
    float        tutTimer;
} GameContext;
