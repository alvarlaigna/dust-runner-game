/*
 * Dust Runner - audio.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "audio.h"
#include <math.h>
#include <stdlib.h>

#define SAMPLE_RATE   22050
#define BUFLEN        1024

/* Tiny PRNG (white noise) */
static float AudioRand01(AudioManager *am) {
    am->rng = am->rng * 1103515245u + 12345u;
    return (float)((am->rng >> 16) & 0xFFFF) / 65535.0f;
}
static float NextNoise(AudioManager *am) { return AudioRand01(am) * 2.0f - 1.0f; }

/* One-shot SFX synthesis */
static Sound SoundFromSamples(short *data, int n) {
    Wave w = { (unsigned int)n, SAMPLE_RATE, 16, 1, data };
    return LoadSoundFromWave(w);   /* copies data; caller frees its buffer */
}

static Sound GenShoot(AudioManager *am) {
    int n = (int)(SAMPLE_RATE * 0.12f);
    short *d = (short *)malloc(n * sizeof(short));
    float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float env = expf(-6.0f * t);
        float freq = 520.0f * (1.0f - 0.6f * t);     /* descending blip */
        ph += freq / SAMPLE_RATE; if (ph >= 1.0f) ph -= 1.0f;
        float sq = (ph < 0.5f) ? 1.0f : -1.0f;
        float s = (sq * 0.6f + NextNoise(am) * 0.4f) * env * 0.5f;
        d[i] = (short)(s * 32767.0f);
    }
    Sound s = SoundFromSamples(d, n); free(d); return s;
}

static Sound GenExplosion(AudioManager *am) {
    int n = (int)(SAMPLE_RATE * 0.5f);
    short *d = (short *)malloc(n * sizeof(short));
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float env = expf(-4.0f * t);
        float rumble = sinf(2.0f * PI * 70.0f * (float)i / SAMPLE_RATE);
        float s = (NextNoise(am) * 0.7f + rumble * 0.5f) * env * 0.6f;
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        d[i] = (short)(s * 32767.0f);
    }
    Sound s = SoundFromSamples(d, n); free(d); return s;
}

static Sound GenHit(AudioManager *am) {
    int n = (int)(SAMPLE_RATE * 0.15f);
    short *d = (short *)malloc(n * sizeof(short));
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float env = expf(-8.0f * t);
        float s = NextNoise(am) * env * 0.5f;
        d[i] = (short)(s * 32767.0f);
    }
    Sound s = SoundFromSamples(d, n); free(d); return s;
}

static Sound GenWaveStart(AudioManager *am) {
    (void)am;
    int n = (int)(SAMPLE_RATE * 0.5f);
    short *d = (short *)malloc(n * sizeof(short));
    float ph1 = 0.0f, ph2 = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float env = sinf(PI * t);                    /* soft bell in/out */
        float f1 = 440.0f + 220.0f * t, f2 = 660.0f + 330.0f * t;  /* rising chime */
        ph1 += f1 / SAMPLE_RATE; if (ph1 >= 1.0f) ph1 -= 1.0f;
        ph2 += f2 / SAMPLE_RATE; if (ph2 >= 1.0f) ph2 -= 1.0f;
        float s = (sinf(2.0f * PI * ph1) + sinf(2.0f * PI * ph2)) * 0.25f * env;
        d[i] = (short)(s * 32767.0f);
    }
    Sound s = SoundFromSamples(d, n); free(d); return s;
}

static Sound GenGameOver(AudioManager *am) {
    int n = (int)(SAMPLE_RATE * 0.8f);
    short *d = (short *)malloc(n * sizeof(short));
    float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float env = (1.0f - t);
        float f = 300.0f * (1.0f - 0.55f * t);       /* descending tone */
        ph += f / SAMPLE_RATE; if (ph >= 1.0f) ph -= 1.0f;
        float s = sinf(2.0f * PI * ph) * env * 0.5f + NextNoise(am) * 0.05f * env;
        d[i] = (short)(s * 32767.0f);
    }
    Sound s = SoundFromSamples(d, n); free(d); return s;
}

static Sound GenMerge(AudioManager *am) {
    (void)am;
    int n = (int)(SAMPLE_RATE * 0.35f);
    short *d = (short *)malloc(n * sizeof(short));
    float ph = 0.0f;
    for (int i = 0; i < n; i++) {
        float t = (float)i / n;
        float env = expf(-3.5f * t) * (1.0f - expf(-40.0f * t));   /* snappy attack */
        float f = (t < 0.5f) ? 620.0f : 930.0f;                    /* two-step rise */
        ph += f / SAMPLE_RATE; if (ph >= 1.0f) ph -= 1.0f;
        float s = (sinf(2.0f * PI * ph) * 0.6f + sinf(2.0f * PI * ph * 2.0f) * 0.2f) * env * 0.5f;
        d[i] = (short)(s * 32767.0f);
    }
    Sound s = SoundFromSamples(d, n); free(d); return s;
}

/* Looping stream fills */
static void FillEngine(AudioManager *am, short *buf, int n) {
    float inc = am->engineFreq / SAMPLE_RATE;
    for (int i = 0; i < n; i++) {
        am->enginePhase += inc; if (am->enginePhase >= 1.0f) am->enginePhase -= 1.0f;
        float saw = am->enginePhase * 2.0f - 1.0f;
        float grit = NextNoise(am) * 0.20f;
        float s = (saw * 0.6f + grit) * am->engineGain;
        if (s > 1.0f) s = 1.0f;
        if (s < -1.0f) s = -1.0f;
        buf[i] = (short)(s * 32767.0f * 0.6f);
    }
}

static void FillWind(AudioManager *am, short *buf, int n) {
    for (int i = 0; i < n; i++) {
        float w = NextNoise(am);
        am->windLP   += (w - am->windLP) * 0.015f;                      /* low-pass */
        am->windGust += ((NextNoise(am) * 0.5f + 0.5f) - am->windGust) * 0.0003f; /* slow gusts */
        float s = am->windLP * (0.35f + 0.65f * am->windGust) * 0.18f;
        buf[i] = (short)(s * 32767.0f);
    }
}

/* Lifecycle */
void AudioManagerInit(AudioManager *am) {
    *am = (AudioManager){0};
    am->rng = 2463534242u;

    am->shoot     = GenShoot(am);
    am->explosion = GenExplosion(am);
    am->hit       = GenHit(am);
    am->waveStart = GenWaveStart(am);
    am->gameOver  = GenGameOver(am);
    am->merge     = GenMerge(am);

    SetAudioStreamBufferSizeDefault(BUFLEN);
    am->engine = LoadAudioStream(SAMPLE_RATE, 16, 1);
    am->wind   = LoadAudioStream(SAMPLE_RATE, 16, 1);
    am->engineFreq = 50.0f; am->engineGain = 0.10f;
    SetAudioStreamVolume(am->engine, 0.42f);   /* -40% vs initial 0.70 */
    SetAudioStreamVolume(am->wind,   0.6f);
    PlayAudioStream(am->engine);
    PlayAudioStream(am->wind);

    am->ready = true;
}

void AudioManagerUpdate(AudioManager *am, float curSpeed, float maxSpeed) {
    if (!am->ready) return;
    float frac = (maxSpeed > 0.0f) ? curSpeed / maxSpeed : 0.0f;
    if (frac > 1.0f) frac = 1.0f;
    am->engineFreq = 48.0f + 95.0f * frac;            /* revs with speed */
    am->engineGain = 0.10f + 0.30f * frac;

    short buf[BUFLEN];
    while (IsAudioStreamProcessed(am->engine)) { FillEngine(am, buf, BUFLEN); UpdateAudioStream(am->engine, buf, BUFLEN); }
    while (IsAudioStreamProcessed(am->wind))   { FillWind(am, buf, BUFLEN);   UpdateAudioStream(am->wind, buf, BUFLEN); }
}

void AudioManagerUnload(AudioManager *am) {
    if (!am->ready) return;
    StopAudioStream(am->engine); StopAudioStream(am->wind);
    UnloadAudioStream(am->engine); UnloadAudioStream(am->wind);
    UnloadSound(am->shoot); UnloadSound(am->explosion); UnloadSound(am->hit);
    UnloadSound(am->waveStart); UnloadSound(am->gameOver); UnloadSound(am->merge);
    am->ready = false;
}

/* Volume control (driven by the pause-menu sliders) */
void AudioSetSfxVolume(AudioManager *am, float v) {
    if (!am->ready) return;
    SetSoundVolume(am->shoot, v);
    SetSoundVolume(am->explosion, v);
    SetSoundVolume(am->hit, v);
    SetSoundVolume(am->waveStart, v);
    SetSoundVolume(am->gameOver, v);
    SetSoundVolume(am->merge, v);
    SetAudioStreamVolume(am->engine, 0.42f * v);   /* engine counts as an SFX */
}
void AudioSetMusicVolume(AudioManager *am, float v) {
    if (!am->ready) return;
    SetAudioStreamVolume(am->wind, 0.6f * v);       /* ambient bed */
}

/* Play helpers */
void AudioPlayShoot(AudioManager *am) {
    if (!am->ready) return;
    SetSoundPitch(am->shoot, 0.9f + 0.2f * AudioRand01(am));   /* slight variation */
    PlaySound(am->shoot);
}
void AudioPlayExplosion(AudioManager *am) { if (am->ready) PlaySound(am->explosion); }
void AudioPlayHit(AudioManager *am)       { if (am->ready) PlaySound(am->hit); }
void AudioPlayWaveStart(AudioManager *am) { if (am->ready) PlaySound(am->waveStart); }
void AudioPlayGameOver(AudioManager *am)  { if (am->ready) PlaySound(am->gameOver); }
void AudioPlayMerge(AudioManager *am) { if (am->ready) PlaySound(am->merge); }
