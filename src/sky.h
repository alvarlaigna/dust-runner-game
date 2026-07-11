/*
 * Dust Runner - sky.h
 * World-space background: day/night clock, gradient sky, sun/moon,
 * parallax mountain/mesa/dune rings, and drifting dust.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "raylib.h"

#define DUST_COUNT 80

typedef struct {
    float     timeOfDay;     /* 0..1, wraps. 0=midnight, 0.5=noon */
    Vector2   sunScreenPos;  /* sun projected to screen, 0..1 */
    float     sunVisible;    /* 1 when sun above horizon, else 0 */
    Color     skyTop;        /* current gradient top */
    Color     skyBottom;     /* current gradient bottom */
    Color     atmosphere;    /* haze / ring tint */
    float     ambient;       /* 0..1 overall brightness */
    Vector3   tint;          /* rgb grade multiplier for post-fx */
    float     haze;          /* heat-haze strength for post-fx */

    Texture2D mtnFar;        /* generated silhouettes */
    Texture2D mtnMid;
    Texture2D dune;
    Vector3   dust[DUST_COUNT]; /* drifting motes */
} SkyState;

void SkyInit(SkyState *s);
void SkyUpdate(SkyState *s, Camera3D cam, float dt);
void SkyDrawBackground(const SkyState *s, Camera3D cam);
void SkyUnload(SkyState *s);
