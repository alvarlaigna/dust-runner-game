/*
 * Dust Runner - hud.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "hud.h"
#include "enemy.h"
#include "menu.h"
#include "input.h"
#include "parts.h"
#include <math.h>
#include <stdio.h>

/* Colour palette */
#define HUD_BG      (Color){10,  10,  10,  180}
#define HUD_ACCENT  (Color){220, 180, 60,  255}
#define HUD_TEXT    (Color){230, 220, 200, 255}
#define HUD_DIM     (Color){140, 130, 110, 200}
#define HUD_RED     (Color){220, 60,  50,  255}
#define HUD_GREEN   (Color){60,  200, 80,  255}

static void DrawPanel(int x, int y, int w, int h, Color bg) {
    DrawRectangle(x, y, w, h, bg);
    DrawRectangleLines(x, y, w, h, HUD_ACCENT);
}

/* Playing HUD */
void HudDrawPlaying(const GameContext *g) {
    char buf[128];

    /* Top-left: wave info */
    DrawPanel(10, 10, 220, 60, HUD_BG);
    snprintf(buf, sizeof(buf), "WAVE  %d", g->wave.number);
    DrawText(buf, 20, 18, 22, HUD_ACCENT);
    snprintf(buf, sizeof(buf), "Enemies: %d", EnemiesAliveCount(g->enemies, ENEMY_MAX) + g->wave.enemiesLeft);
    DrawText(buf, 20, 44, 16, HUD_TEXT);

    /* Top-right: score / scrap */
    DrawPanel(SCREEN_W - 230, 10, 220, 60, HUD_BG);
    snprintf(buf, sizeof(buf), "Score: %d", g->score);
    DrawText(buf, SCREEN_W - 220, 18, 18, HUD_TEXT);
    snprintf(buf, sizeof(buf), "Scrap: %d", g->scrapCollected);
    DrawText(buf, SCREEN_W - 220, 42, 16, HUD_ACCENT);

    /* Bottom-left: HP bar */
    int barX = 10, barY = SCREEN_H - 40;
    DrawPanel(barX, barY, 260, 30, HUD_BG);
    float hpFrac = (float)g->vehicle.hp / (float)g->vehicle.hpMax;
    DrawRectangle(barX + 4, barY + 4, (int)((252) * hpFrac), 22,
                  hpFrac > 0.5f ? HUD_GREEN :
                  hpFrac > 0.25f ? (Color){220, 180, 30, 255} : HUD_RED);
    snprintf(buf, sizeof(buf), "HP  %d / %d", g->vehicle.hp, g->vehicle.hpMax);
    DrawText(buf, barX + 8, barY + 7, 16, HUD_TEXT);

    /* Bottom-right: auto-aim indicator */
    const char *aimStr = g->vehicle.autoAim ? "[SPACE] Auto-Aim: ON" : "[SPACE] Auto-Aim: OFF";
    Color aimCol = g->vehicle.autoAim ? HUD_GREEN : HUD_DIM;
    DrawText(aimStr, SCREEN_W - 230, SCREEN_H - 48, 15, aimCol);

}

/* Respite screen */
void HudDrawRespite(const GameContext *g) {
    char buf[128];
    DrawPanel(SCREEN_W/2 - 160, SCREEN_H/2 - 50, 320, 100, HUD_BG);
    snprintf(buf, sizeof(buf), "WAVE %d CLEAR!", g->wave.number);
    int tw = MeasureText(buf, 28);
    DrawText(buf, SCREEN_W/2 - tw/2, SCREEN_H/2 - 40, 28, HUD_ACCENT);
    snprintf(buf, sizeof(buf), "Next wave in  %.0fs", g->wave.respiteTimer);
    tw = MeasureText(buf, 20);
    DrawText(buf, SCREEN_W/2 - tw/2, SCREEN_H/2, 20, HUD_TEXT);
    snprintf(buf, sizeof(buf), "Kills: %d   Score: %d", g->kills, g->score);
    tw = MeasureText(buf, 16);
    DrawText(buf, SCREEN_W/2 - tw/2, SCREEN_H/2 + 28, 16, HUD_DIM);
}

/* Category colour shared by the rig hexes and the draft cards. */
static Color RigCatColor(UpgradeCategory c) {
    return c == UPCAT_WEAPON  ? (Color){200, 70,  55,  255} :
           c == UPCAT_CHASSIS ? (Color){60,  130, 210, 255} :
                                (Color){210, 170, 60,  255};
}

/* 6-slot honeycomb, top-right, below the score panel. Tune positions visually. */
void HudDrawRig(const GameContext *g) {
    const float r = 20.0f;
    const float baseX = SCREEN_W - 86.0f, baseY = 96.0f;
    for (int i = 0; i < RIG_SLOTS; i++) {
        int col = i / 3, row = i % 3;
        float cx = baseX + col * (r * 1.7f);
        float cy = baseY + row * (r * 1.75f) + (col ? r * 0.9f : 0.0f);
        Vector2 c = { cx, cy };
        const HexSlot *s = &g->vehicle.slots[i];

        Color fill, line;
        if (!s->filled) {
            fill = (Color){ 30, 28, 24, 150 };
            line = (Color){ 90, 84, 70, 200 };
        } else {
            fill = RigCatColor(PartGetDef(s->type)->category);
            line = (Color){ 245, 235, 210, 255 };
        }
        if (g->rigFlashTimer > 0.0f && g->rigFlashSlot == i) {
            float f = g->rigFlashTimer / RIG_FLASH_TIME;
            fill.r = (unsigned char)fminf(255.0f, fill.r + 120.0f * f);
            fill.g = (unsigned char)fminf(255.0f, fill.g + 120.0f * f);
            fill.b = (unsigned char)fminf(255.0f, fill.b + 120.0f * f);
        }
        DrawPoly(c, 6, r, 30.0f, fill);
        DrawPolyLinesEx(c, 6, r, 30.0f, 2.0f, line);
        if (s->filled) {
            char lv[4]; snprintf(lv, sizeof lv, "%d", s->level);
            int tw = MeasureText(lv, 18);
            DrawText(lv, (int)(cx - tw / 2), (int)(cy - 9), 18, (Color){ 20, 18, 14, 255 });
        }
    }
}

Rectangle HudDraftSkipRect(void) {
    return (Rectangle){ SCREEN_W/2 - 90, SCREEN_H - 58, 180, 38 };
}

/* Part-choice cards (replaces the old upgrade draft). */
void HudDrawDraft(GameContext *g) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 0, 0, 0, 160 });

    const char *title = "INSTALL A PART";
    int tw = MeasureText(title, 34);
    DrawText(title, SCREEN_W/2 - tw/2, 80, 34, HUD_ACCENT);

    int cardW = UPGRADE_CARD_W, cardH = UPGRADE_CARD_H, gap = UPGRADE_CARD_GAP;
    int totalW = UPGRADE_CHOICES * cardW + (UPGRADE_CHOICES - 1) * gap;
    int startX = SCREEN_W/2 - totalW/2;
    int cardY  = SCREEN_H/2 - cardH/2 + 10;
    Vector2 mouse = InputPointerPosition();

    for (int i = 0; i < UPGRADE_CHOICES; i++) {
        const PartDef *d = PartGetDef(g->rigChoices[i]);
        int cx = startX + i * (cardW + gap);
        bool hover = (mouse.x >= cx && mouse.x <= cx + cardW &&
                      mouse.y >= cardY && mouse.y <= cardY + cardH);
        if (hover) g->upgradeHover = i;

        DrawRectangle(cx, cardY, cardW, cardH, hover ? (Color){40,35,20,230} : HUD_BG);
        DrawRectangleLines(cx, cardY, cardW, cardH, hover ? HUD_ACCENT : (Color){100,90,70,255});

        const char *cat = d->category == UPCAT_WEAPON  ? "WEAPON"  :
                          d->category == UPCAT_CHASSIS ? "CHASSIS" : "UTILITY";
        DrawRectangle(cx + 8, cardY + 8, MeasureText(cat, 14) + 12, 22, RigCatColor(d->category));
        DrawText(cat, cx + 14, cardY + 12, 14, WHITE);

        DrawText(d->name, cx + 10, cardY + 40, 20, HUD_TEXT);
        DrawText("Install Lv1", cx + 10, cardY + 68, 14, HUD_DIM);

        char l3[48];
        snprintf(l3, sizeof l3, "Lv3: %s", d->l3unlock);
        DrawText(l3, cx + 10, cardY + 90, 14, HUD_ACCENT);

        char key[4] = { (char)('1' + i), '\0', '\0', '\0' };
        DrawText(key, cx + cardW/2 - 6, cardY + cardH - 28, 22, hover ? HUD_ACCENT : HUD_DIM);
    }

    Rectangle skip = HudDraftSkipRect();
    bool skHover = CheckCollisionPointRec(mouse, skip);
    DrawRectangleRec(skip, skHover ? (Color){50,44,30,230} : HUD_BG);
    DrawRectangleLinesEx(skip, 1.0f, skHover ? HUD_ACCENT : (Color){100,90,70,255});
    const char *sk = "[S] Skip";
    int stw = MeasureText(sk, 18);
    DrawText(sk, (int)(skip.x + skip.width/2 - stw/2), (int)(skip.y + 9), 18, HUD_TEXT);

    DrawText("Press 1 / 2 / 3 to install  -  three of a kind merge up",
             SCREEN_W/2 - 220, SCREEN_H - 92, 16, HUD_DIM);
}

/* Game over screen */
void HudDrawGameOver(const GameContext *g) {
    char buf[128];
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){0, 0, 0, 200});

    const char *title = "DUST RUNNER";
    int tw = MeasureText(title, 60);
    DrawText(title, SCREEN_W/2 - tw/2, SCREEN_H/2 - 140, 60, HUD_ACCENT);

    const char *sub = "YOU HAVE FALLEN";
    tw = MeasureText(sub, 30);
    DrawText(sub, SCREEN_W/2 - tw/2, SCREEN_H/2 - 70, 30, HUD_RED);

    snprintf(buf, sizeof(buf), "Wave reached: %d", g->wave.number);
    tw = MeasureText(buf, 22);
    DrawText(buf, SCREEN_W/2 - tw/2, SCREEN_H/2 - 20, 22, HUD_TEXT);

    snprintf(buf, sizeof(buf), "Kills: %d   Score: %d   Scrap: %d",
             g->kills, g->score, g->scrapCollected);
    tw = MeasureText(buf, 18);
    DrawText(buf, SCREEN_W/2 - tw/2, SCREEN_H/2 + 14, 18, HUD_DIM);

    const char *restart = "Press  R  to restart";
    tw = MeasureText(restart, 24);
    DrawText(restart, SCREEN_W/2 - tw/2, SCREEN_H/2 + 60, 24, HUD_ACCENT);

    Rectangle mm = { SCREEN_W/2 - 140, SCREEN_H/2 + 96, 280, 46 };
    MenuButton(mm, "MAIN MENU", true, false);
    int cw = MeasureText(GAME_CREDIT, 14);
    DrawText(GAME_CREDIT, SCREEN_W/2 - cw/2, SCREEN_H - 26, 14, HUD_DIM);
}

/* True on click/release of the game-over Main Menu button. */
bool HudGameOverMainMenu(void) {
    Rectangle mm = { SCREEN_W/2 - 140, SCREEN_H/2 + 96, 280, 46 };
    return CheckCollisionPointRec(InputPointerPosition(), mm) && InputPointerReleased();
}

/* First-run tutorial prompt (contextual). */
void HudDrawTutorial(const GameContext *g, bool touch) {
    if (g->tutStep == TUT_DONE) return;
    const char *msg;
    if (g->tutStep == TUT_MOVE) {
        msg = touch ? "Tap the ground to move" : "Click the ground to move";
    } else { /* TUT_FIRE: only prompt once an enemy is near, so it reads as contextual */
        if (!EnemyFindNearest((Enemy *)g->enemies, ENEMY_MAX, g->vehicle.pos, TURRET_RANGE)) return;
        msg = touch ? "Enemies are auto-targeted, keep moving to survive"
                    : "Right-click to fire, or SPACE for auto-aim";
    }
    int fs = 20, tw = MeasureText(msg, fs);
    int pad = 16, w = tw + pad*2, h = fs + pad;
    int x = SCREEN_W/2 - w/2, y = SCREEN_H - 118;
    DrawRectangle(x, y, w, h, (Color){ 10, 10, 10, 180 });
    DrawRectangleLines(x, y, w, h, (Color){ 220, 180, 60, 255 });
    DrawText(msg, x + pad, y + pad/2, fs, (Color){ 235, 220, 200, 255 });
}

/* On-screen touch controls; rects shared so hud draw + main hit-test agree. */
Rectangle HudTouchPauseRect(void)   { return (Rectangle){ SCREEN_W - 62, 78,  48, 48 }; }
Rectangle HudTouchAutoAimRect(void) { return (Rectangle){ SCREEN_W - 62, 132, 48, 48 }; }

void HudDrawTouchControls(const GameContext *g) {
    Rectangle p = HudTouchPauseRect();
    DrawRectangleRounded(p, 0.25f, 6, (Color){ 20, 18, 14, 180 });
    DrawRectangle((int)p.x + 16, (int)p.y + 14, 5, 20, RAYWHITE);
    DrawRectangle((int)p.x + 27, (int)p.y + 14, 5, 20, RAYWHITE);   /* pause glyph */

    Rectangle a = HudTouchAutoAimRect();
    bool on = g->vehicle.autoAim;
    DrawRectangleRounded(a, 0.25f, 6, on ? (Color){ 60, 140, 70, 200 } : (Color){ 40, 36, 32, 180 });
    DrawText("AA", (int)a.x + 12, (int)a.y + 15, 20, on ? RAYWHITE : (Color){ 150, 140, 120, 255 });
}
