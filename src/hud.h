/*
 * Dust Runner - hud.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

void HudDrawPlaying(const GameContext *g);
void HudDrawRespite(const GameContext *g);
void HudDrawUpgrade(GameContext *g);   /* mutates upgradeHover */
void HudDrawGameOver(const GameContext *g);
bool HudGameOverMainMenu(void);   /* true when the game-over Main Menu button is clicked */
void HudDrawTutorial(const GameContext *g, bool touch);

/* On-screen touch controls (shared rects for draw + hit-test). */
Rectangle HudTouchPauseRect(void);
Rectangle HudTouchAutoAimRect(void);
void      HudDrawTouchControls(const GameContext *g);
