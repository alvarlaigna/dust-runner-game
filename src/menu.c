/*
 * Dust Runner - menu.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "menu.h"
#include "game.h"     /* SCREEN_W / SCREEN_H */
#include "input.h"
#include <string.h>   /* memcpy for settings persistence */

#define SETTINGS_FILE  "dustrunner_settings.dat"
#define SETTINGS_MAGIC 0x44525331u   /* "DRS1"; bump if the Settings layout changes */

static const int   FPS_OPTIONS[] = { 30, 60, 120, 144, 0 };   /* 0 = unlimited */
static const char *FPS_LABELS[]  = { "30", "60", "120", "144", "Off" };
#define FPS_OPTION_COUNT 5

/* Layout (fixed, computed from screen size) */
typedef struct {
    Rectangle panel;
    Rectangle aaToggle;
    Rectangle fps[FPS_OPTION_COUNT];
    Rectangle master, sounds, music;
    Rectangle resume, exitBtn;
} MenuLayout;

static MenuLayout Layout(void) {
    MenuLayout L;
    float pw = 560.0f, ph = 430.0f;
    float px = (SCREEN_W - pw) * 0.5f;
    float py = (SCREEN_H - ph) * 0.5f;
    L.panel    = (Rectangle){ px, py, pw, ph };
    L.aaToggle = (Rectangle){ px + 380, py + 78, 120, 34 };
    float fx = px + 200, fy = py + 138, bw = 62, gap = 6;
    for (int i = 0; i < FPS_OPTION_COUNT; i++)
        L.fps[i] = (Rectangle){ fx + i * (bw + gap), fy, bw, 34 };
    L.master  = (Rectangle){ px + 200, py + 212, 250, 14 };
    L.sounds  = (Rectangle){ px + 200, py + 267, 250, 14 };
    L.music   = (Rectangle){ px + 200, py + 322, 250, 14 };
    L.resume  = (Rectangle){ px + 80,  py + 388, 180, 48 };
    L.exitBtn = (Rectangle){ px + 300, py + 388, 180, 48 };
    return L;
}

void SettingsInit(Settings *s) {
    s->aa = true;
    s->fpsIndex = 1;          /* 60 fps */
    s->masterVol = 1.0f;
    s->sfxVol = 1.0f;
    s->musicVol = 0.6f;
    s->quit = false;
}

int SettingsTargetFps(const Settings *s) {
    int i = s->fpsIndex;
    if (i < 0 || i >= FPS_OPTION_COUNT) i = 1;
    return FPS_OPTIONS[i];
}

/* Load persisted settings (defaults if missing or mismatched). Desktop persists;
 * on web the file lives in the in-memory FS and does not survive a reload. */
void SettingsLoad(Settings *s) {
    SettingsInit(s);
    if (!FileExists(SETTINGS_FILE)) return;
    int size = 0;
    unsigned char *data = LoadFileData(SETTINGS_FILE, &size);
    if (data && size == (int)(sizeof(unsigned int) + sizeof(Settings))) {
        unsigned int magic;
        memcpy(&magic, data, sizeof(magic));
        if (magic == SETTINGS_MAGIC) memcpy(s, data + sizeof(magic), sizeof(Settings));
    }
    if (data) UnloadFileData(data);
    s->quit = false;   /* never persist the quit flag */
}

void SettingsSave(const Settings *s) {
    unsigned char buf[sizeof(unsigned int) + sizeof(Settings)];
    unsigned int magic = SETTINGS_MAGIC;
    memcpy(buf, &magic, sizeof(magic));
    memcpy(buf + sizeof(magic), s, sizeof(Settings));
    SaveFileData(SETTINGS_FILE, buf, (int)sizeof(buf));
}

/* Input */
static float SliderInput(Rectangle track, Vector2 m, bool down, float v) {
    Rectangle hit = { track.x - 6, track.y - 10, track.width + 12, track.height + 20 };
    if (down && CheckCollisionPointRec(m, hit)) {
        v = (m.x - track.x) / track.width;
        if (v < 0.0f) v = 0.0f;
        if (v > 1.0f) v = 1.0f;
    }
    return v;
}

/* Shared menu button: warm-ochre primary, hover/press, dimmed when disabled.
 * Returns true on a left-click release inside an enabled button. */
bool MenuButton(Rectangle r, const char *label, bool enabled, bool primary) {
    Vector2 m = InputPointerPosition();
    bool hover = enabled && CheckCollisionPointRec(m, r);
    bool held  = hover && InputPointerDown();
    Color bg = !enabled ? (Color){ 40, 36, 32, 220 }
             : held     ? (Color){ 235, 180, 80, 255 }
             : primary  ? (Color){ 200, 150, 60, 255 }
             : hover    ? (Color){ 86, 76, 60, 255 }
                        : (Color){ 60, 54, 46, 255 };
    DrawRectangleRounded(r, 0.18f, 6, bg);
    DrawRectangleRoundedLinesEx(r, 0.18f, 6, 1.0f, (Color){ 120, 100, 70, 255 });
    int fs = 22, tw = MeasureText(label, fs);
    Color fg = !enabled ? (Color){ 120, 110, 96, 255 }
             : (primary || held) ? (Color){ 30, 24, 16, 255 } : RAYWHITE;
    DrawText(label, (int)(r.x + (r.width - tw) * 0.5f), (int)(r.y + (r.height - fs) * 0.5f), fs, fg);
    return hover && InputPointerReleased();
}

/* Settings controls only (no Resume/Exit); reused by pause and title. */
void SettingsPanelUpdate(Settings *s) {
    MenuLayout L = Layout();
    Vector2 m = InputPointerPosition();
    bool click = InputPointerPressed();
    bool down  = InputPointerDown();

    if (click && CheckCollisionPointRec(m, L.aaToggle)) s->aa = !s->aa;
    for (int i = 0; i < FPS_OPTION_COUNT; i++)
        if (click && CheckCollisionPointRec(m, L.fps[i])) s->fpsIndex = i;

    s->masterVol = SliderInput(L.master, m, down, s->masterVol);
    s->sfxVol    = SliderInput(L.sounds, m, down, s->sfxVol);
    s->musicVol  = SliderInput(L.music,  m, down, s->musicVol);
}

/* Drawing */
static void DrawButton(Rectangle r, const char *t, bool hot) {
    bool hover = CheckCollisionPointRec(InputPointerPosition(), r);
    Color bg = hot   ? (Color){ 200, 150, 60, 255 }
             : hover ? (Color){ 86, 76, 60, 255 }
                     : (Color){ 60, 54, 46, 255 };
    DrawRectangleRec(r, bg);
    DrawRectangleLinesEx(r, 1.0f, (Color){ 120, 100, 70, 255 });
    int fs = 20, tw = MeasureText(t, fs);
    DrawText(t, (int)(r.x + (r.width - tw) * 0.5f), (int)(r.y + (r.height - fs) * 0.5f), fs,
             hot ? (Color){ 30, 24, 16, 255 } : RAYWHITE);
}

static void DrawSlider(Rectangle track, const char *label, float v) {
    DrawText(label, (int)(track.x - 170), (int)(track.y - 4), 20, (Color){ 210, 200, 180, 255 });
    DrawRectangleRec(track, (Color){ 50, 46, 40, 255 });
    DrawRectangle((int)track.x, (int)track.y, (int)(track.width * v), (int)track.height,
                  (Color){ 200, 150, 60, 255 });
    DrawRectangleLinesEx(track, 1.0f, (Color){ 120, 100, 70, 255 });
    DrawCircle((int)(track.x + track.width * v), (int)(track.y + track.height * 0.5f), 9, RAYWHITE);
    DrawText(TextFormat("%d%%", (int)(v * 100.0f)), (int)(track.x + track.width + 14),
             (int)(track.y - 4), 20, RAYWHITE);
}

/* Settings panel body only; callers supply the dim + surrounding chrome. */
void SettingsPanelDraw(const Settings *s) {
    MenuLayout L = Layout();
    DrawRectangleRec(L.panel, (Color){ 28, 24, 20, 238 });
    DrawRectangleLinesEx(L.panel, 2.0f, (Color){ 120, 100, 70, 255 });
    DrawText("SETTINGS", (int)(L.panel.x + 30), (int)(L.panel.y + 22), 30, (Color){ 235, 215, 160, 255 });

    DrawText("Anti-aliasing", (int)(L.panel.x + 30), (int)(L.aaToggle.y + 7), 20, (Color){ 210, 200, 180, 255 });
    DrawButton(L.aaToggle, s->aa ? "On" : "Off", s->aa);

    DrawText("Frame limit", (int)(L.panel.x + 30), (int)(L.fps[0].y + 7), 20, (Color){ 210, 200, 180, 255 });
    for (int i = 0; i < FPS_OPTION_COUNT; i++)
        DrawButton(L.fps[i], FPS_LABELS[i], i == s->fpsIndex);

    DrawSlider(L.master, "Master", s->masterVol);
    DrawSlider(L.sounds, "Sounds", s->sfxVol);
    DrawSlider(L.music,  "Music",  s->musicVol);
}

/* Pause menu with a Settings sub-view. */
typedef enum { PAUSE_ROOT = 0, PAUSE_SETTINGS } PauseView;
static PauseView gPauseView = PAUSE_ROOT;

typedef struct { Rectangle cont, settings, quitTitle, quit, back; } PauseLayout;
static PauseLayout PauseLayoutCompute(void) {
    PauseLayout L;
    float bw = 300, bh = 50, gap = 12;
    float x  = (SCREEN_W - bw) * 0.5f;
    float y0 = SCREEN_H * 0.34f;
    L.cont      = (Rectangle){ x, y0 + 0*(bh+gap), bw, bh };
    L.settings  = (Rectangle){ x, y0 + 1*(bh+gap), bw, bh };
    L.quitTitle = (Rectangle){ x, y0 + 2*(bh+gap), bw, bh };
    L.quit      = (Rectangle){ x, y0 + 3*(bh+gap), bw, bh };
    L.back      = (Rectangle){ x, SCREEN_H - 90.0f, bw, bh };
    return L;
}

void PauseReset(void)      { gPauseView = PAUSE_ROOT; }
bool PauseInSettings(void) { return gPauseView == PAUSE_SETTINGS; }

MenuAction PauseUpdate(Settings *s) {
    PauseLayout L = PauseLayoutCompute();
    if (gPauseView == PAUSE_SETTINGS) {
        SettingsPanelUpdate(s);
        if (MenuButton(L.back, "BACK", true, false) || IsKeyPressed(KEY_ESCAPE)) gPauseView = PAUSE_ROOT;
        return MENU_NONE;
    }
    if (MenuButton(L.cont, "CONTINUE", true, true))            return MENU_CONTINUE;
    if (MenuButton(L.settings, "SETTINGS", true, false))       gPauseView = PAUSE_SETTINGS;
    if (MenuButton(L.quitTitle, "QUIT TO TITLE", true, false)) return MENU_QUIT_TITLE;
#if !defined(PLATFORM_WEB)
    if (MenuButton(L.quit, "QUIT", true, false))               return MENU_QUIT;
#endif
    return MENU_NONE;
}

void PauseDraw(const Settings *s) {
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 0, 0, 0, 150 });
    PauseLayout L = PauseLayoutCompute();
    if (gPauseView == PAUSE_SETTINGS) {
        SettingsPanelDraw(s);
        MenuButton(L.back, "BACK", true, false);
        return;
    }
    const char *t = "PAUSED";
    int tw = MeasureText(t, 40);
    DrawText(t, SCREEN_W/2 - tw/2, (int)(SCREEN_H*0.22f), 40, (Color){ 235, 215, 160, 255 });
    MenuButton(L.cont, "CONTINUE", true, true);
    MenuButton(L.settings, "SETTINGS", true, false);
    MenuButton(L.quitTitle, "QUIT TO TITLE", true, false);
#if !defined(PLATFORM_WEB)
    MenuButton(L.quit, "QUIT", true, false);
#endif
}

/* Title screen */
typedef struct { Rectangle start, cont, settings, quit; } TitleLayout;

static TitleLayout TitleLayoutCompute(void) {
    TitleLayout L;
    float bw = 280, bh = 52, gap = 14;
    float x  = (SCREEN_W - bw) * 0.5f;
    float y0 = SCREEN_H * 0.46f;
    L.start    = (Rectangle){ x, y0 + 0*(bh+gap), bw, bh };
    L.cont     = (Rectangle){ x, y0 + 1*(bh+gap), bw, bh };
    L.settings = (Rectangle){ x, y0 + 2*(bh+gap), bw, bh };
    L.quit     = (Rectangle){ x, y0 + 3*(bh+gap), bw, bh };
    return L;
}

MenuAction TitleUpdate(bool runActive) {
    TitleLayout L = TitleLayoutCompute();
    if (MenuButton(L.start, "START", true, true))            return MENU_START;
    if (MenuButton(L.cont, "CONTINUE", runActive, false))    return MENU_CONTINUE;
    if (MenuButton(L.settings, "SETTINGS", true, false))     return MENU_SETTINGS;
#if !defined(PLATFORM_WEB)
    if (MenuButton(L.quit, "QUIT", true, false))             return MENU_QUIT;
#endif
    return MENU_NONE;
}

void TitleDraw(bool runActive) {
    TitleLayout L = TitleLayoutCompute();
    /* Darken the live diorama a touch for legibility. */
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 0, 0, 0, 70 });

    const char *title = "DUST RUNNER";
    int ts = 64, tw = MeasureText(title, ts);
    DrawText(title, SCREEN_W/2 - tw/2 + 2, (int)(SCREEN_H*0.22f) + 2, ts, (Color){ 20, 14, 8, 180 }); /* shadow */
    DrawText(title, SCREEN_W/2 - tw/2, (int)(SCREEN_H*0.22f), ts, (Color){ 235, 200, 120, 255 });
    const char *tag = "tactical desert survival";
    int gs = 20, gw = MeasureText(tag, gs);
    DrawText(tag, SCREEN_W/2 - gw/2, (int)(SCREEN_H*0.22f) + ts + 6, gs, (Color){ 200, 180, 150, 220 });

    MenuButton(L.start, "START", true, true);
    MenuButton(L.cont, "CONTINUE", runActive, false);
    MenuButton(L.settings, "SETTINGS", true, false);
#if !defined(PLATFORM_WEB)
    MenuButton(L.quit, "QUIT", true, false);
#endif

    int cw = MeasureText(GAME_CREDIT, 16);
    DrawText(GAME_CREDIT, SCREEN_W/2 - cw/2, SCREEN_H - 30, 16, (Color){ 170, 150, 120, 220 });
}
