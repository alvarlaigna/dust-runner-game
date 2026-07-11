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

void TerrainDraw(const Terrain *t) {
    for (int y = 0; y < TERRAIN_TILES; y++) {
        for (int x = 0; x < TERRAIN_TILES; x++) {
            TileType tt = t->tiles[y][x];
            Color c = FadeEdge(TILE_COLOR[tt], x, y);
            Vector3 pos = {
                x * TILE_SIZE + TILE_SIZE * 0.5f,
                0.0f,
                y * TILE_SIZE + TILE_SIZE * 0.5f
            };
            DrawCube(pos, TILE_SIZE - 0.05f, 0.1f, TILE_SIZE - 0.05f, c);

            /* Ruin: draw a slightly raised block */
            if (tt == TILE_RUIN) {
                Vector3 rp = { pos.x, 0.6f, pos.z };
                DrawCube(rp, TILE_SIZE * 0.7f, 1.2f, TILE_SIZE * 0.7f,
                         (Color){70, 62, 50, 255});
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
