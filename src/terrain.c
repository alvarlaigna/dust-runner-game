/*
 * Dust Runner - terrain.c
 * Procedural desert terrain using a lightweight value-noise approach.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "terrain.h"
#include <math.h>
#include <string.h>

/* Minimal value-noise (no external deps) */
static unsigned int _seed;

static float Hash2(int x, int y) {
    unsigned int h = (unsigned int)(x * 1619 + y * 31337 + _seed * 6271);
    h ^= h >> 16; h *= 0x45d9f3b; h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.0f;
}

static float SmoothNoise(float fx, float fy) {
    int ix = (int)floorf(fx), iy = (int)floorf(fy);
    float tx = fx - ix, ty = fy - iy;
    /* smoothstep */
    tx = tx * tx * (3.0f - 2.0f * tx);
    ty = ty * ty * (3.0f - 2.0f * ty);
    float a = Hash2(ix,   iy);
    float b = Hash2(ix+1, iy);
    float c = Hash2(ix,   iy+1);
    float d = Hash2(ix+1, iy+1);
    return a + (b-a)*tx + (c-a)*ty + (a-b-c+d)*tx*ty;
}

static float Fbm(float fx, float fy, int octaves) {
    float val = 0.0f, amp = 0.5f, freq = 1.0f;
    for (int i = 0; i < octaves; i++) {
        val  += SmoothNoise(fx * freq, fy * freq) * amp;
        amp  *= 0.5f;
        freq *= 2.0f;
    }
    return val;
}

/* Terrain generation */
void TerrainGenerate(Terrain *t, unsigned int seed) {
    _seed = seed;
    memset(t, 0, sizeof(*t));

    for (int y = 0; y < TERRAIN_TILES; y++) {
        for (int x = 0; x < TERRAIN_TILES; x++) {
            float nx = (float)x / TERRAIN_TILES * 6.0f;
            float ny = (float)y / TERRAIN_TILES * 6.0f;
            float v  = Fbm(nx, ny, 4);

            TileType tile;
            if      (v > 0.72f) tile = TILE_RUIN;
            else if (v > 0.58f) tile = TILE_HARDPAN;
            else if (v > 0.35f) tile = TILE_SAND;
            else                tile = TILE_DUNE;

            /* Keep a safe spawn zone clear around centre */
            int cx = TERRAIN_TILES/2, cy = TERRAIN_TILES/2;
            int dx = x - cx, dy = y - cy;
            if (dx*dx + dy*dy < 25) tile = TILE_HARDPAN;

            t->tiles[y][x] = tile;
        }
    }

    /* Poisson-ish scrap placement */
    t->scrapCount = 0;
    for (int i = 0; i < 64 && t->scrapCount < 48; i++) {
        float sx = Hash2(i * 7, seed & 0xFF) * TERRAIN_WORLD;
        float sz = Hash2(i * 13, (seed >> 8) & 0xFF) * TERRAIN_WORLD;
        /* reject if too close to another scrap or to centre */
        float dcx = sx - TERRAIN_WORLD * 0.5f;
        float dcz = sz - TERRAIN_WORLD * 0.5f;
        if (dcx*dcx + dcz*dcz < 400.0f) continue;
        bool tooClose = false;
        for (int j = 0; j < t->scrapCount; j++) {
            float ddx = t->scrapPos[j].x - sx;
            float ddz = t->scrapPos[j].z - sz;
            if (ddx*ddx + ddz*ddz < 64.0f) { tooClose = true; break; }
        }
        if (!tooClose) {
            t->scrapPos[t->scrapCount] = (Vector3){ sx, 0.5f, sz };
            t->scrapActive[t->scrapCount] = true;
            t->scrapCount++;
        }
    }
}

/* Drawing */
#define EDGE_FADE_TILES 7
static const Color EDGE_HAZE = { 190, 168, 125, 255 };   /* matches the outer skirt */

static Color FadeEdge(Color c, int x, int y) {
    int bd = x;
    if (y < bd) bd = y;
    if (TERRAIN_TILES - 1 - x < bd) bd = TERRAIN_TILES - 1 - x;
    if (TERRAIN_TILES - 1 - y < bd) bd = TERRAIN_TILES - 1 - y;
    if (bd >= EDGE_FADE_TILES) return c;
    float f = (float)bd / (float)EDGE_FADE_TILES;   /* 0 at border, 1 inland */
    return (Color){
        (unsigned char)(EDGE_HAZE.r + (c.r - EDGE_HAZE.r) * f),
        (unsigned char)(EDGE_HAZE.g + (c.g - EDGE_HAZE.g) * f),
        (unsigned char)(EDGE_HAZE.b + (c.b - EDGE_HAZE.b) * f),
        255,
    };
}

/* Cheap per-tile pseudo-random in [0,1) for height variation (deterministic). */
static float TileHash01(int x, int y) {
    unsigned int h = ((unsigned int)x * 73856093u) ^ ((unsigned int)y * 19349663u);
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return (float)(h & 0xFFFF) / 65536.0f;
}

void TerrainDraw(const Terrain *t, Vector3 center) {
    /* Camera-local culling window (edge-fade/haze hides the boundary). */
    int cx = (int)(center.x / TILE_SIZE), cy = (int)(center.z / TILE_SIZE);
    int x0 = cx - TERRAIN_CULL, x1 = cx + TERRAIN_CULL;
    int y0 = cy - TERRAIN_CULL, y1 = cy + TERRAIN_CULL;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= TERRAIN_TILES) x1 = TERRAIN_TILES - 1;
    if (y1 >= TERRAIN_TILES) y1 = TERRAIN_TILES - 1;

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            TileType tt = t->tiles[y][x];
            Color c = FadeEdge(TILE_COLOR[tt], x, y);
            float px = x * TILE_SIZE + TILE_SIZE * 0.5f;
            float pz = y * TILE_SIZE + TILE_SIZE * 0.5f;

            if (tt == TILE_RUIN) {
                /* ruin: tall dark hex pillar (cover) */
                DrawCylinder((Vector3){ px, 0.0f, pz }, HEX_R * 0.85f, HEX_R * 0.85f,
                             HEX_RUIN_H, 6, (Color){ 70, 62, 50, 255 });
            } else {
                int lvl = (int)(TileHash01(x, y) * 3.0f);            /* 0..2 jagged levels */
                float h = HEX_H_MIN + (float)lvl * HEX_H_STEP;
                DrawCylinder((Vector3){ px, 0.0f, pz }, HEX_R, HEX_R, h, 6, c);
            }
        }
    }

    /* Scrap piles */
    for (int i = 0; i < t->scrapCount; i++) {
        if (!t->scrapActive[i]) continue;
        DrawCube(t->scrapPos[i], 0.8f, 0.8f, 0.8f, (Color){180, 130, 60, 255});
        DrawCubeWires(t->scrapPos[i], 0.8f, 0.8f, 0.8f, (Color){220, 180, 80, 255});
    }
}

/* Helpers */
static void WorldToTile(Vector3 pos, int *tx, int *ty) {
    *tx = (int)(pos.x / TILE_SIZE);
    *ty = (int)(pos.z / TILE_SIZE);
}

float TerrainSpeedMod(const Terrain *t, Vector3 pos) {
    int tx, ty;
    WorldToTile(pos, &tx, &ty);
    if (tx < 0 || tx >= TERRAIN_TILES || ty < 0 || ty >= TERRAIN_TILES)
        return 1.0f;
    return TILE_SPEED_MOD[t->tiles[ty][tx]];
}

bool TerrainIsPassable(const Terrain *t, Vector3 pos) {
    int tx, ty;
    WorldToTile(pos, &tx, &ty);
    if (tx < 0 || tx >= TERRAIN_TILES || ty < 0 || ty >= TERRAIN_TILES)
        return false;
    return t->tiles[ty][tx] != TILE_RUIN;
}
