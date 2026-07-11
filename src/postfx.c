/*
 * Dust Runner - postfx.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "postfx.h"
#include <stdio.h>   /* snprintf */

/* Post-process fragment shader - one effect body compiled for two GLSL targets:
 * desktop GL 3.3 (#version 330) and WebGL/GLES2 (#version 100).
 * Effects: heat haze, tilt-shift, FXAA-lite, god rays, color grade, vignette. */
#if defined(PLATFORM_WEB)
    #define GLSL_VERSION 100
#else
    #define GLSL_VERSION 330
#endif

/* Platform header: declares varying/out + compatibility macros so the body below
 * is written once and compiles on both desktop GL 3.3 and WebGL/GLES2. */
#if GLSL_VERSION == 100
static const char *POSTFX_HEADER =
"#version 100\n"
"#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
"precision highp float;\n"
"#else\n"
"precision mediump float;\n"
"#endif\n"
"#define VIN varying\n"
"#define SAMPLE texture2D\n"
"#define FRAGCOLOR gl_FragColor\n";
#else
static const char *POSTFX_HEADER =
"#version 330\n"
"#define VIN in\n"
"#define SAMPLE texture\n"
"out vec4 fragColorOut;\n"
"#define FRAGCOLOR fragColorOut\n";
#endif

/* Shared effect body (uses VIN/SAMPLE/FRAGCOLOR from the header above). */
static const char *POSTFX_BODY =
"VIN vec2 fragTexCoord;\n"
"uniform sampler2D texture0;\n"
"uniform vec2  uResolution;\n"
"uniform float uTime;\n"
"uniform vec2  uSunPos;\n"
"uniform float uSunVisible;\n"
"uniform vec3  uTint;\n"
"uniform float uAmbient;\n"
"uniform float uHaze;\n"
"uniform float uAA;\n"
"void main(){\n"
"    vec2 uv = fragTexCoord;\n"
"    float horizon = smoothstep(0.15, 0.55, uv.y);\n"
"    float amt = uHaze * 0.0030 * (0.4 + horizon);\n"
"    uv.x += sin(uv.y * 42.0 + uTime * 3.0) * cos(uv.x * 13.0 - uTime * 1.7) * amt;\n"
"    float ts = smoothstep(0.28, 0.5, abs(uv.y - 0.5)) * 0.004;\n"
"    vec3 col = SAMPLE(texture0, uv).rgb;\n"
"    col += SAMPLE(texture0, uv + vec2(0.0,  ts)).rgb;\n"
"    col += SAMPLE(texture0, uv + vec2(0.0, -ts)).rgb;\n"
"    col /= 3.0;\n"
"    if (uAA > 0.5) {\n"
"        vec2 px = 1.0 / uResolution;\n"
"        vec3 n = SAMPLE(texture0, uv + vec2(0.0, -px.y)).rgb;\n"
"        vec3 s = SAMPLE(texture0, uv + vec2(0.0,  px.y)).rgb;\n"
"        vec3 e = SAMPLE(texture0, uv + vec2( px.x, 0.0)).rgb;\n"
"        vec3 w = SAMPLE(texture0, uv + vec2(-px.x, 0.0)).rgb;\n"
"        vec3 L = vec3(0.299, 0.587, 0.114);\n"
"        float lc = dot(col, L);\n"
"        float edge = abs(dot(n,L)-lc) + abs(dot(s,L)-lc) + abs(dot(e,L)-lc) + abs(dot(w,L)-lc);\n"
"        col = mix(col, (col + n + s + e + w) / 5.0, clamp(edge * 3.0, 0.0, 0.65));\n"
"    }\n"
"    if (uSunVisible > 0.5) {\n"
"        vec2 dir = (uSunPos - uv) / 12.0 * 0.7;\n"
"        vec2 sp = uv; float illum = 1.0; vec3 ray = vec3(0.0);\n"
"        for (int i = 0; i < 12; i++) {\n"
"            sp += dir; vec3 c = SAMPLE(texture0, sp).rgb;\n"
"            float b = max(0.0, dot(c, vec3(0.299, 0.587, 0.114)) - 0.6);\n"
"            ray += c * b * illum * 0.10; illum *= 0.92;\n"
"        }\n"
"        col += ray * 1.4;\n"
"    }\n"
"    col *= uTint;\n"
"    col *= (0.55 + 0.45 * uAmbient);\n"
"    col = pow(max(col, vec3(0.0)), vec3(0.95));\n"
"    vec2 d = uv - 0.5;\n"
"    col *= mix(0.72, 1.0, smoothstep(0.95, 0.30, length(d) * 1.3));\n"
"    FRAGCOLOR = vec4(col, 1.0);\n"
"}\n";

void PostFXInit(PostFX *fx, int width, int height) {
    *fx = (PostFX){0};
    fx->width = width; fx->height = height;
    fx->target = LoadRenderTexture(width, height);
    SetTextureFilter(fx->target.texture, TEXTURE_FILTER_BILINEAR);
    static char src[4096];
    snprintf(src, sizeof(src), "%s%s", POSTFX_HEADER, POSTFX_BODY);
    fx->shader = LoadShaderFromMemory(0, src);
    fx->locTime       = GetShaderLocation(fx->shader, "uTime");
    fx->locResolution = GetShaderLocation(fx->shader, "uResolution");
    fx->locSunPos     = GetShaderLocation(fx->shader, "uSunPos");
    fx->locSunVisible = GetShaderLocation(fx->shader, "uSunVisible");
    fx->locTint       = GetShaderLocation(fx->shader, "uTint");
    fx->locAmbient    = GetShaderLocation(fx->shader, "uAmbient");
    fx->locHaze       = GetShaderLocation(fx->shader, "uHaze");
    fx->locAA         = GetShaderLocation(fx->shader, "uAA");
    /* sensible defaults so the scene is unmodified until mood is set */
    fx->tint = (Vector3){1.0f, 1.0f, 1.0f};
    fx->ambient = 1.0f;
    fx->sunVisible = 0.0f;
    fx->haze = 0.0f;
    fx->aa = 1.0f;
}

void PostFXSetAA(PostFX *fx, float on) { fx->aa = on; }

void PostFXSetMood(PostFX *fx, Vector2 sunPos, float sunVisible, Vector3 tint, float ambient, float haze) {
    fx->sunPos = sunPos;
    fx->sunVisible = sunVisible;
    fx->tint = tint;
    fx->ambient = ambient;
    fx->haze = haze;
}

void PostFXBegin(PostFX *fx) {
    BeginTextureMode(fx->target);
    ClearBackground((Color){18, 14, 10, 255});
}

void PostFXEnd(PostFX *fx) {
    (void)fx;
    EndTextureMode();
}

void PostFXDraw(PostFX *fx, float dt) {
    fx->time += dt;
    float res[2] = { (float)fx->width, (float)fx->height };
    SetShaderValue(fx->shader, fx->locResolution, res, SHADER_UNIFORM_VEC2);
    SetShaderValue(fx->shader, fx->locTime, &fx->time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fx->shader, fx->locSunPos, &fx->sunPos, SHADER_UNIFORM_VEC2);
    SetShaderValue(fx->shader, fx->locSunVisible, &fx->sunVisible, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fx->shader, fx->locTint, &fx->tint, SHADER_UNIFORM_VEC3);
    SetShaderValue(fx->shader, fx->locAmbient, &fx->ambient, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fx->shader, fx->locHaze, &fx->haze, SHADER_UNIFORM_FLOAT);
    SetShaderValue(fx->shader, fx->locAA, &fx->aa, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(fx->shader);
    /* Flip vertically (negative source height) per raylib RenderTexture convention. */
    DrawTextureRec(fx->target.texture,
        (Rectangle){ 0, 0, (float)fx->target.texture.width, -(float)fx->target.texture.height },
        (Vector2){ 0, 0 }, WHITE);
    EndShaderMode();
}

void PostFXUnload(PostFX *fx) {
    UnloadShader(fx->shader);
    UnloadRenderTexture(fx->target);
}
