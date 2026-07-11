/*
 * Dust Runner - hud.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

void HudDrawPlaying(const GameContext *g);
void HudDrawRespite(const GameContext *g);
void HudDrawRig(const GameContext *g);        /* 6-hex honeycomb of installed parts */
void HudDrawDraft(GameContext *g);            /* part-choice cards; mutates upgradeHover */
Rectangle HudDraftSkipRect(void);             /* shared Skip-button rect */
void HudDrawGameOver(const GameContext *g);
bool HudGameOverMainMenu(void);   /* true when the game-over Main Menu button is clicked */
void HudDrawTutorial(const GameContext *g, bool touch);

/* On-screen touch controls (shared rects for draw + hit-test). */
Rectangle HudTouchPauseRect(void);
Rectangle HudTouchAutoAimRect(void);
void      HudDrawTouchControls(const GameContext *g);
