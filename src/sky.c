/*
 * Dust Runner - sky.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "sky.h"
#include "game.h"      /* SCREEN_W/H, TERRAIN_WORLD */
#include "raymath.h"
#include "rlgl.h"
#include <math.h>

#define DAY_LENGTH_SECONDS  300.0f   /* full day/night cycle */
#define WORLD_CENTER        (TERRAIN_WORLD * 0.5f)

/* Small helpers */
static Color ColorLerpC(Color a, Color b, float t) {
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t),
    };
}

/* Day/night keyframes at t = 0(midnight) .25(sunrise) .5(noon) .75(sunset) → wraps to 0. */
static const Color KEY_TOP[4]  = {
    {  18,  20,  40, 255 },   /* midnight: deep blue */
    { 230, 140,  90, 255 },   /* sunrise: orange */
    { 120, 165, 220, 255 },   /* noon: pale blue */
    { 220, 110,  80, 255 },   /* sunset: red-orange */
};
static const Color KEY_BOT[4]  = {
    {  30,  28,  45, 255 },   /* midnight */
    { 245, 200, 150, 255 },   /* sunrise: warm haze */
    { 225, 210, 180, 255 },   /* noon: bright sand-sky */
    { 245, 190, 140, 255 },   /* sunset */
};
static const Color KEY_ATMO[4] = {
    {  60,  60,  90, 255 },
    { 210, 160, 130, 255 },
    { 200, 180, 150, 255 },
    { 205, 150, 120, 255 },
};
static const float KEY_AMB[4]  = { 0.30f, 0.70f, 1.00f, 0.65f };

static void SamplePalette(float t, Color *top, Color *bot, Color *atmo, float *amb) {
    float ft = t * 4.0f;             /* 0..4 */
    int i = (int)ft & 3;             /* 0..3 */
    int j = (i + 1) & 3;
    float f = ft - floorf(ft);
    *top  = ColorLerpC(KEY_TOP[i],  KEY_TOP[j],  f);
    *bot  = ColorLerpC(KEY_BOT[i],  KEY_BOT[j],  f);
    *atmo = ColorLerpC(KEY_ATMO[i], KEY_ATMO[j], f);
    *amb  = KEY_AMB[i] + (KEY_AMB[j] - KEY_AMB[i]) * f;
}

/* Deterministic 1-D value noise for silhouette ridgelines. */
static float Hash1(int x, unsigned int seed) {
    unsigned int h = (unsigned int)x * 374761393u + seed * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.0f;
}
static float Ridge(float x, unsigned int seed) {
    int ix = (int)floorf(x);
    float f = x - ix;
    f = f * f * (3.0f - 2.0f * f);
    float a = Hash1(ix, seed), b = Hash1(ix + 1, seed);
    return a + (b - a) * f;
}

/* Generate a wide silhouette: opaque below a noisy ridgeline, alpha-faded near
 * the ridge for atmospheric haze. `rough` = ridge frequency, `tall` = 0..1 height. */
static Texture2D GenSilhouette(int w, int h, Color base, unsigned int seed,
                               float rough, float tall) {
    Image img = GenImageColor(w, h, (Color){ 0, 0, 0, 0 });
    for (int x = 0; x < w; x++) {
        float fx = (float)x / w * rough;
        float n = Ridge(fx, seed) * 0.6f + Ridge(fx * 2.3f, seed + 7) * 0.3f
                + Ridge(fx * 5.1f, seed + 19) * 0.1f;
        int top = (int)((1.0f - tall * (0.45f + 0.55f * n)) * h);
        if (top < 0) top = 0;
        for (int y = top; y < h; y++) {
            float vf = (float)(y - top) / (float)(h - top + 1); /* 0 ridge..1 base */
            Color c = base;
            c.r = (unsigned char)(base.r * (0.65f + 0.35f * vf));
            c.g = (unsigned char)(base.g * (0.65f + 0.35f * vf));
            c.b = (unsigned char)(base.b * (0.65f + 0.35f * vf));
            c.a = (unsigned char)(150 + (int)(105 * vf));        /* soft top edge */
            ImageDrawPixel(&img, x, y, c);
        }
    }
    Texture2D t = LoadTextureFromImage(img);
    SetTextureFilter(t, TEXTURE_FILTER_BILINEAR);
    UnloadImage(img);
    return t;
}

/* Lifecycle */
void SkyInit(SkyState *s) {
    *s = (SkyState){0};
    s->timeOfDay = 0.35f;            /* start mid-morning */
    s->mtnFar = GenSilhouette(512, 256, (Color){120, 110, 130, 255}, 11u, 8.0f,  0.75f);
    s->mtnMid = GenSilhouette(512, 256, (Color){135, 110,  95, 255}, 29u, 14.0f, 0.55f);
    s->dune   = GenSilhouette(512, 192, (Color){200, 170, 120, 255}, 53u, 22.0f, 0.32f);

    for (int i = 0; i < DUST_COUNT; i++) {
        s->dust[i] = (Vector3){
            WORLD_CENTER + (Hash1(i, 101u) - 0.5f) * 120.0f,
            2.0f + Hash1(i, 202u) * 10.0f,
            WORLD_CENTER + (Hash1(i, 303u) - 0.5f) * 120.0f,
        };
    }
}

void SkyUpdate(SkyState *s, Camera3D cam, float dt) {
    s->timeOfDay += dt / DAY_LENGTH_SECONDS;
    if (s->timeOfDay >= 1.0f) s->timeOfDay -= 1.0f;

    SamplePalette(s->timeOfDay, &s->skyTop, &s->skyBottom, &s->atmosphere, &s->ambient);

    /* Grade tint: warm by day, cool by night, derived from atmosphere + ambient. */
    s->tint = (Vector3){
        (0.55f + 0.45f * s->ambient) * (s->atmosphere.r / 255.0f) + 0.45f,
        (0.55f + 0.45f * s->ambient) * (s->atmosphere.g / 255.0f) + 0.40f,
        (0.55f + 0.45f * s->ambient) * (s->atmosphere.b / 255.0f) + 0.40f,
    };
    s->haze = 0.4f + 0.6f * s->ambient;   /* more shimmer in the heat of day */

    /* Sun arc: rises in +X, sets in -X, biased toward the visible -Z horizon. */
    float ang = s->timeOfDay * 2.0f * PI;
    Vector3 dir = {
        cosf(ang - PI * 0.5f),    /* horizontal sweep */
        sinf(ang),                /* elevation: >0 day, <0 night */
        -0.6f,                    /* bias to the far (visible) side */
    };
    dir = Vector3Normalize(dir);
    s->sunVisible = (dir.y > 0.02f) ? 1.0f : 0.0f;

    Vector3 disc = dir;
    if (s->sunVisible < 0.5f) disc = Vector3Negate(dir);   /* moon = opposite point */
    Vector3 worldPos = Vector3Add(cam.target, Vector3Scale(disc, 600.0f));
    Vector2 sp = GetWorldToScreen(worldPos, cam);
    s->sunScreenPos = (Vector2){ sp.x / (float)SCREEN_W, sp.y / (float)SCREEN_H };

    /* Wind drift; wrap within a box centred on the camera target. */
    for (int i = 0; i < DUST_COUNT; i++) {
        s->dust[i].x += 6.0f * dt;
        s->dust[i].z += 2.0f * dt;
        float bx = s->dust[i].x - cam.target.x;
        float bz = s->dust[i].z - cam.target.z;
        if (bx >  60.0f) s->dust[i].x -= 120.0f;
        if (bz >  60.0f) s->dust[i].z -= 120.0f;
        if (bx < -60.0f) s->dust[i].x += 120.0f;
        if (bz < -60.0f) s->dust[i].z += 120.0f;
    }
}

void SkyUnload(SkyState *s) {
    if (s->mtnFar.id) UnloadTexture(s->mtnFar);
    if (s->mtnMid.id) UnloadTexture(s->mtnMid);
    if (s->dune.id)   UnloadTexture(s->dune);
}

/* Background draw */
/* Draw billboards along a square border `half` units out from world-center, so
 * the silhouettes frame the (square) map edges rather than a circle's corners. */
static void DrawSkyBorder(Camera3D cam, Texture2D tex, float half, float y,
                          float segW, float segH, int perSide, Color tint) {
    Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
    float c = WORLD_CENTER;
    for (int side = 0; side < 4; side++) {
        for (int i = 0; i < perSide; i++) {
            float u = c + (((float)i + 0.5f) / perSide * 2.0f - 1.0f) * half;
            Vector3 p;
            switch (side) {
                case 0:  p = (Vector3){ u, y, c - half }; break;  /* far  (-z) */
                case 1:  p = (Vector3){ u, y, c + half }; break;  /* near (+z) */
                case 2:  p = (Vector3){ c - half, y, u }; break;  /* left (-x) */
                default: p = (Vector3){ c + half, y, u }; break;  /* right(+x) */
            }
            DrawBillboardRec(cam, tex, src, p, (Vector2){ segW, segH }, tint);
        }
    }
}

void SkyDrawBackground(const SkyState *s, Camera3D cam) {
    /* 2D sky behind the 3D scene: disable depth so it never rejects geometry. */
    rlDisableDepthTest();
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, s->skyTop, s->skyBottom);

    /* Sun / moon disc with soft glow, in screen space. */
    Vector2 c = { s->sunScreenPos.x * SCREEN_W, s->sunScreenPos.y * SCREEN_H };
    bool day = s->sunVisible > 0.5f;
    Color core = day ? (Color){ 255, 240, 200, 255 } : (Color){ 220, 225, 240, 255 };
    Color glow = day ? (Color){ 255, 200, 120,  60 } : (Color){ 180, 190, 220,  50 };
    for (int i = 4; i >= 1; i--) DrawCircleV(c, 26.0f * i, glow);
    DrawCircleV(c, 26.0f, core);

    rlEnableDepthTest();

    BeginMode3D(cam);

    /* Outer desert skirt: faded ground extending well beyond the playfield so the
     * world reads as larger than the map. Drawn just below the map plane. */
    Color skirt = (Color){ 190, 168, 125, 255 };
    DrawPlane((Vector3){ WORLD_CENTER, -0.2f, WORLD_CENTER }, (Vector2){ 1400.0f, 1400.0f }, skirt);

    /* Parallax rings around the map border: far → near, far most haze-tinted. */
    Color atmo = s->atmosphere;
    Color farTint = (Color){ (unsigned char)((atmo.r + 255) / 2),
                             (unsigned char)((atmo.g + 235) / 2),
                             (unsigned char)((atmo.b + 235) / 2), 230 };
    DrawSkyBorder(cam, s->mtnFar, 235.0f, 18.0f,  95.0f, 46.0f, 10, farTint);
    DrawSkyBorder(cam, s->mtnMid, 200.0f, 11.0f,  72.0f, 30.0f, 11, (Color){235,225,210,240});
    DrawSkyBorder(cam, s->dune,   172.0f,  5.0f,  52.0f, 14.0f, 12, (Color){245,235,215,255});

    /* Drifting dust motes (additive, reuses the dune texture as a soft sprite). */
    BeginBlendMode(BLEND_ADDITIVE);
    for (int i = 0; i < DUST_COUNT; i++) {
        float sz = 0.5f + Hash1(i, 404u) * 0.8f;
        DrawBillboard(cam, s->dune, s->dust[i], sz, (Color){200, 180, 140, 40});
    }
    EndBlendMode();
    EndMode3D();
}
