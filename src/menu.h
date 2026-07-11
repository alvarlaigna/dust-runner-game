/*
 * Dust Runner - menu.h
 * Pause menu + persistent settings (AA, frame limit, volumes, exit).
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "raylib.h"
#include <stdbool.h>

typedef struct {
    bool  aa;          /* anti-aliasing (FXAA-lite) */
    int   fpsIndex;    /* index into the frame-limit option table */
    float masterVol;   /* 0..1 */
    float sfxVol;      /* 0..1 */
    float musicVol;    /* 0..1 */
    bool  quit;        /* set by the Exit button */
} Settings;

void SettingsInit(Settings *s);
int  SettingsTargetFps(const Settings *s);   /* resolves fpsIndex -> fps (0 = unlimited) */
void SettingsLoad(Settings *s);              /* load persisted settings, or defaults */
void SettingsSave(const Settings *s);        /* persist settings (desktop) */

/* Menu action intent (menu.c reports; main.c routes to screen changes) */
typedef enum {
    MENU_NONE = 0, MENU_START, MENU_CONTINUE, MENU_SETTINGS, MENU_QUIT_TITLE, MENU_QUIT
} MenuAction;

/* Shared warm-ochre button widget; returns true on a click-release inside it. */
bool       MenuButton(Rectangle r, const char *label, bool enabled, bool primary);

/* Title screen (drawn over the live diorama). */
MenuAction TitleUpdate(bool runActive);
void       TitleDraw(bool runActive);

/* Shared settings panel (pause + title Settings views). */
void       SettingsPanelUpdate(Settings *s);
void       SettingsPanelDraw(const Settings *s);

/* Pause menu: Continue / Settings / Quit to Title / Quit. */
MenuAction PauseUpdate(Settings *s);
void       PauseDraw(const Settings *s);
void       PauseReset(void);
bool       PauseInSettings(void);
