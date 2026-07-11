/*
 * Dust Runner - audio.h
 * Procedural audio: synthesized one-shot SFX + real-time engine/wind streams.
 * No external asset files. InitAudioDevice() must be called before AudioManagerInit.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "raylib.h"
#include <stdbool.h>

typedef struct {
    /* one-shot SFX (synthesized at init) */
    Sound       shoot, explosion, hit, waveStart, gameOver, merge;
    /* looping procedural streams */
    AudioStream engine, wind;
    bool        ready;
    /* synth state */
    float        engineFreq, engineGain, enginePhase;
    float        windLP, windGust;
    unsigned int rng;
} AudioManager;

void AudioManagerInit(AudioManager *am);                               /* needs InitAudioDevice() first */
void AudioManagerUpdate(AudioManager *am, float curSpeed, float maxSpeed);
void AudioManagerUnload(AudioManager *am);

void AudioSetSfxVolume(AudioManager *am, float v);    /* one-shot SFX + engine */
void AudioSetMusicVolume(AudioManager *am, float v);  /* ambient bed (wind) */

void AudioPlayShoot(AudioManager *am);
void AudioPlayExplosion(AudioManager *am);
void AudioPlayHit(AudioManager *am);
void AudioPlayWaveStart(AudioManager *am);
void AudioPlayGameOver(AudioManager *am);
void AudioPlayMerge(AudioManager *am);
