/*
 * Dust Runner - postfx.h
 * Screen-space post-process pipeline: offscreen render target + shader blit.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "raylib.h"

typedef struct {
    RenderTexture2D target;
    Shader  shader;
    int     locTime, locResolution, locSunPos, locSunVisible, locTint, locAmbient, locHaze, locAA;
    float   time;
    float   aa;           /* 0/1 anti-aliasing (FXAA-lite) toggle */
    int     width, height;
    /* per-frame mood (set via PostFXSetMood) */
    Vector2 sunPos;       /* 0..1 screen space */
    float   sunVisible;   /* 0 or 1 */
    Vector3 tint;         /* rgb grade multiplier */
    float   ambient;      /* 0..1 */
    float   haze;         /* heat-haze strength */
} PostFX;

void PostFXInit(PostFX *fx, int width, int height);
void PostFXSetMood(PostFX *fx, Vector2 sunPos, float sunVisible, Vector3 tint, float ambient, float haze);
void PostFXSetAA(PostFX *fx, float on);
void PostFXBegin(PostFX *fx);   /* begin rendering the scene into the target */
void PostFXEnd(PostFX *fx);     /* end scene rendering */
void PostFXDraw(PostFX *fx, float dt); /* blit target to screen through the shader */
void PostFXUnload(PostFX *fx);
