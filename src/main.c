/*
 * Dust Runner - main.c
 * Entry point, app-screen routing, per-frame update/draw, gameplay glue.
 *
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 *
 * Controls: LMB move, RMB fire, SPACE auto-aim, ESC pause, R restart.
 */

#include "game.h"
#include "terrain.h"
#include "vehicle.h"
#include "enemy.h"
#include "projectile.h"
#include "wave.h"
#include "parts.h"
#include "hud.h"
#include "postfx.h"
#include "sky.h"
#include "audio.h"
#include "menu.h"
#include "input.h"

#include <string.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif

/* Audio manager - owned by main(), referenced by gameplay hooks. Kept out of
 * GameContext because GameInit memsets the context on every restart. */
static AudioManager *gAudio = NULL;

/* All app state, reachable by the UpdateDrawFrame() emscripten callback.
 * GPU/audio resources are owned here, never in GameInit() (which memsets g). */
typedef struct {
    Settings     settings;
    AudioManager audio;
    PostFX       fx;
    SkyState     sky;
    GameContext  g;

    /* App-level screen state - lives here (not in GameContext) so it survives
     * GameInit's memset on every fresh run / restart. */
    AppScreen screen;
    bool      runActive;        /* is there a resumable run? gates Continue */
    bool      touchSeen;        /* a touch was detected; enables on-screen mobile controls */
    /* fade transition */
    bool      onTransition, transFadeOut;
    float     transAlpha;
    AppScreen transFrom, transTo;
} App;
static App gApp;

/* Title Settings sub-view flag (shared by UpdateDrawFrame + GameDraw). */
static bool gTitleSettings = false;

/* Forward declarations */
static void GameInit(GameContext *g);
static void GameUpdate(GameContext *g, float dt);
static void GameDraw(GameContext *g, SkyState *sky, PostFX *fx, const Settings *settings);
static Vector3 GroundRaycast(Camera3D cam, Vector2 mousePos);
static void TryFireTurret(GameContext *g, Vector3 targetDir);
static void UpdateDrawFrame(void);

/* Entry point */
int main(void) {
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_W, SCREEN_H,
               "Dust Runner - by Alvar Laigna | alvarlaigna.itch.io");
    SetExitKey(KEY_NULL);          /* ESC opens the pause menu, doesn't quit */
    SetTargetFPS(TARGET_FPS);

    SettingsLoad(&gApp.settings);

    InitAudioDevice();
    AudioManagerInit(&gApp.audio);
    gAudio = &gApp.audio;

    PostFXInit(&gApp.fx, SCREEN_W, SCREEN_H);
    SkyInit(&gApp.sky);
    GameInit(&gApp.g);
    gApp.screen = SCREEN_TITLE;   /* boot into the title over the live diorama */

#if defined(PLATFORM_WEB)
    /* The browser owns the frame loop and the tab lifetime; this never returns,
     * so the desktop teardown below is intentionally skipped on web. */
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    while (!WindowShouldClose() && !gApp.settings.quit) {
        UpdateDrawFrame();
    }

    SettingsSave(&gApp.settings);   /* persist settings on desktop exit */
    gAudio = NULL;
    SkyUnload(&gApp.sky);
    PostFXUnload(&gApp.fx);
    AudioManagerUnload(&gApp.audio);
    CloseAudioDevice();
    CloseWindow();
#endif
    return 0;
}

/* App-screen fade transitions + always-on overlays */
static void TransitionToScreen(AppScreen to) {
    gApp.onTransition = true; gApp.transFadeOut = true;
    gApp.transFrom = gApp.screen; gApp.transTo = to; gApp.transAlpha = 0.0f;
}
static void UpdateTransition(void) {
    const float SPD = 2.6f; /* ~0.38s each way */
    if (gApp.transFadeOut) {
        gApp.transAlpha += SPD * GetFrameTime();
        if (gApp.transAlpha >= 1.0f) { gApp.transAlpha = 1.0f; gApp.screen = gApp.transTo; gApp.transFadeOut = false; }
    } else {
        gApp.transAlpha -= SPD * GetFrameTime();
        if (gApp.transAlpha <= 0.0f) { gApp.transAlpha = 0.0f; gApp.onTransition = false; }
    }
}
static void DrawTransition(void) {
    if (gApp.onTransition)
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 0, 0, 0, (unsigned char)(gApp.transAlpha * 255.0f) });
}
static void DrawVersion(void) {
    int w = MeasureText(GAME_VERSION, 14);
    DrawText(GAME_VERSION, SCREEN_W - w - 8, SCREEN_H - 18, 14, (Color){ 150, 135, 110, 180 });
}

/* Per-frame update+draw (also the emscripten main-loop callback) */
static void UpdateDrawFrame(void) {
    App *a = &gApp;
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f; /* cap at 20 fps equivalent */

    InputBeginFrame();

    if (GetTouchPointCount() > 0) a->touchSeen = true;

    /* Apply settings every frame (cheap, idempotent). */
    SetTargetFPS(SettingsTargetFps(&a->settings));
    SetMasterVolume(a->settings.masterVol);
    AudioSetSfxVolume(&a->audio, a->settings.sfxVol);
    AudioSetMusicVolume(&a->audio, a->settings.musicVol);
    PostFXSetAA(&a->fx, a->settings.aa ? 1.0f : 0.0f);

    if (a->onTransition) {
        UpdateTransition();
        SkyUpdate(&a->sky, a->g.camera, dt);          /* keep the backdrop alive during fades */
    } else if (a->screen == SCREEN_TITLE) {
        MenuAction act = gTitleSettings ? MENU_NONE : TitleUpdate(a->runActive);
        if (act == MENU_START)    { GameInit(&a->g); a->runActive = true;
                                    if (a->touchSeen) a->g.vehicle.autoAim = true;
                                    TransitionToScreen(SCREEN_GAMEPLAY); }
        if (act == MENU_CONTINUE && a->runActive)      TransitionToScreen(SCREEN_GAMEPLAY);
        if (act == MENU_QUIT)     a->settings.quit = true;
        if (act == MENU_SETTINGS) gTitleSettings = true;
        if (gTitleSettings) {
            SettingsPanelUpdate(&a->settings);
            Rectangle back = { SCREEN_W/2 - 150, SCREEN_H - 88, 300, 50 };
            if (MenuButton(back, "BACK", true, false) || IsKeyPressed(KEY_ESCAPE)) gTitleSettings = false;
        }
        a->g.camera.target.x += (0.0f - a->g.camera.target.x) * 0.02f; /* slow drift to center */
        SkyUpdate(&a->sky, a->g.camera, dt);
    } else { /* SCREEN_GAMEPLAY */
        if (IsKeyPressed(KEY_ESCAPE) && !PauseInSettings()) a->g.paused = !a->g.paused;
        if (a->g.paused) {
            MenuAction act = PauseUpdate(&a->settings);
            if (act == MENU_CONTINUE)   { a->g.paused = false; PauseReset(); }
            if (act == MENU_QUIT_TITLE) { a->g.paused = false; PauseReset(); TransitionToScreen(SCREEN_TITLE); }
            if (act == MENU_QUIT)       a->settings.quit = true;
        } else {
            /* On-screen touch controls (pause / auto-aim), only while playing. */
            if (a->touchSeen && a->g.state == STATE_PLAYING && InputPointerReleased()) {
                Vector2 mp = InputPointerPosition();
                if (CheckCollisionPointRec(mp, HudTouchPauseRect()))   a->g.paused = true;
                if (CheckCollisionPointRec(mp, HudTouchAutoAimRect())) a->g.vehicle.autoAim = !a->g.vehicle.autoAim;
            }
            GameUpdate(&a->g, dt);
            SkyUpdate(&a->sky, a->g.camera, dt);
            if (a->g.state == STATE_GAMEOVER) {
                a->runActive = false;
                if (HudGameOverMainMenu()) TransitionToScreen(SCREEN_TITLE);
            }
        }
    }

    AudioManagerUpdate(&a->audio,
        (a->screen == SCREEN_GAMEPLAY && !a->g.paused) ? a->g.vehicle.curSpeed : 0.0f,
        a->g.vehicle.speed);
    PostFXSetMood(&a->fx, a->sky.sunScreenPos, a->sky.sunVisible, a->sky.tint, a->sky.ambient, a->sky.haze);
    GameDraw(&a->g, &a->sky, &a->fx, &a->settings);
}

/* Initialise / restart */
static void GameInit(GameContext *g) {
    memset(g, 0, sizeof(*g));

    /* Camera: fixed isometric-style perspective */
    g->camera.position   = (Vector3){ 0, 55, 55 };
    g->camera.target     = (Vector3){ 0, 0, 0 };
    g->camera.up         = (Vector3){ 0, 1, 0 };
    g->camera.fovy       = CAM_FOVY;   /* 1:1 framing (see game.h) */
    g->camera.projection = CAMERA_PERSPECTIVE;

    TerrainGenerate(&g->terrain, (unsigned int)GetTime() * 1000 + 42);
    VehicleInit(&g->vehicle);
    WaveInit(&g->wave);
    srand((unsigned int)time(NULL));

    g->state = STATE_PLAYING;
    WaveStartNext(&g->wave);
}

/* Ground-plane raycast */
static Vector3 GroundRaycast(Camera3D cam, Vector2 mousePos) {
    Ray ray = GetScreenToWorldRay(mousePos, cam);
    /* Intersect with Y=0 plane */
    if (fabsf(ray.direction.y) < 0.0001f) return (Vector3){0,0,0};
    float t = -ray.position.y / ray.direction.y;
    return (Vector3){
        ray.position.x + ray.direction.x * t,
        0.0f,
        ray.position.z + ray.direction.z * t
    };
}

/* Fire turret toward a world-space direction */
static void TryFireTurret(GameContext *g, Vector3 worldTarget) {
    Vehicle *v = &g->vehicle;
    if (v->fireCooldown > 0.0f) return;

    Vector3 dir = Vector3Subtract(worldTarget, v->pos);
    dir.y = 0;
    float len = Vector3Length(dir);
    if (len < 0.1f) return;
    dir = Vector3Normalize(dir);

    /* Barrel tip offset */
    Vector3 barrelTip = {
        v->pos.x + dir.x * 1.8f,
        1.2f,
        v->pos.z + dir.z * 1.8f
    };

    ProjectileFire(g->projectiles, PROJ_MAX, barrelTip, dir, v);
    v->fireCooldown = 1.0f / v->fireRate;
    if (gAudio) AudioPlayShoot(gAudio);
}

/* Camera follow */
static void CameraFollow(Camera3D *cam, Vector3 target, float dt) {
    float lerpSpeed = 4.0f;
    float t = 1.0f - expf(-lerpSpeed * dt);
    cam->target.x += (target.x - cam->target.x) * t;
    cam->target.z += (target.z - cam->target.z) * t;
    cam->position.x = cam->target.x;
    cam->position.z = cam->target.z + CAM_BACK;   /* top-down; retuned for 1:1 */
    cam->position.y = CAM_HEIGHT;
}

/* Scrap collection */
static void CollectScrap(GameContext *g) {
    Vehicle *v = &g->vehicle;
    float pickupR = v->hasMagnet ? v->magnetRadius : 2.5f;
    for (int i = 0; i < g->terrain.scrapCount; i++) {
        if (!g->terrain.scrapActive[i]) continue;
        float dx = g->terrain.scrapPos[i].x - v->pos.x;
        float dz = g->terrain.scrapPos[i].z - v->pos.z;
        if (dx*dx + dz*dz < pickupR * pickupR) {
            g->terrain.scrapActive[i] = false;
            g->scrapCollected += 5;
            g->score += 10;
        }
    }
}

/* Main update */
static void GameUpdate(GameContext *g, float dt) {
    g->runTime += dt;

    /* Game over state */
    if (g->state == STATE_GAMEOVER) {
        if (IsKeyPressed(KEY_R)) GameInit(g);
        return;
    }

    /* Rig draft state */
    if (g->state == STATE_UPGRADE) {
        int chosen = -1;
        bool skip = false;
        if (IsKeyPressed(KEY_ONE))   chosen = 0;
        if (IsKeyPressed(KEY_TWO))   chosen = 1;
        if (IsKeyPressed(KEY_THREE)) chosen = 2;
        if (IsKeyPressed(KEY_S))     skip = true;

        if (InputPointerPressed()) {
            Vector2 m = InputPointerPosition();
            if (CheckCollisionPointRec(m, HudDraftSkipRect())) {
                skip = true;
            } else {
                int cardW = UPGRADE_CARD_W, cardH = UPGRADE_CARD_H, gap = UPGRADE_CARD_GAP;
                int totalW = UPGRADE_CHOICES * cardW + (UPGRADE_CHOICES - 1) * gap;
                int startX = SCREEN_W/2 - totalW/2;
                int cardY  = SCREEN_H/2 - cardH/2 + 10;
                for (int i = 0; i < UPGRADE_CHOICES; i++) {
                    int cx = startX + i * (cardW + gap);
                    if (m.x >= cx && m.x <= cx + cardW &&
                        m.y >= cardY && m.y <= cardY + cardH) { chosen = i; break; }
                }
            }
        }

        if (chosen >= 0 && chosen < UPGRADE_CHOICES) {
            int flash = -1;
            int merges = PartsDraft(g->vehicle.slots, g->rigChoices[chosen], &flash);
            VehicleRecomputeStats(&g->vehicle);
            if (merges > 0) {
                g->rigFlashTimer = RIG_FLASH_TIME;
                g->rigFlashSlot  = flash;
                if (gAudio) AudioPlayMerge(gAudio);
            }
            g->state = STATE_PLAYING;
            WaveStartNext(&g->wave);
            if (gAudio) AudioPlayWaveStart(gAudio);
        } else if (skip) {
            g->state = STATE_PLAYING;
            WaveStartNext(&g->wave);
            if (gAudio) AudioPlayWaveStart(gAudio);
        }
        return;
    }

    /* Respite state */
    if (g->state == STATE_RESPITE) {
        g->wave.respiteTimer -= dt;
        if (g->wave.respiteTimer <= 0.0f) {
            /* Show upgrade screen */
            PartsRollChoices(g->rigChoices, UPGRADE_CHOICES);
            g->upgradeHover = 0;
            g->state = STATE_UPGRADE;
        }
        return;
    }

    /* Playing state */
    Vehicle *v = &g->vehicle;
    if (g->rigFlashTimer > 0.0f) g->rigFlashTimer -= dt;

    /* Toggle auto-aim */
    if (IsKeyPressed(KEY_SPACE)) v->autoAim = !v->autoAim;

    /* Tutorial: advance on the taught action. */
    if (g->tutStep == TUT_MOVE && InputPointerPressed()) {
        g->tutStep = TUT_FIRE; g->tutTimer = 0.0f;
    } else if (g->tutStep == TUT_FIRE) {
        g->tutTimer += dt;
        bool fired = IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) || IsKeyPressed(KEY_SPACE);
        bool touchAdvance = (GetTouchPointCount() > 0 && g->tutTimer > 4.0f);
        if (fired || touchAdvance) g->tutStep = TUT_DONE;
    }

    /* LMB: move command (skip taps that land on the on-screen touch controls) */
    if (InputPointerPressed()) {
        Vector2 mp = InputPointerPosition();
        bool onTouchUI = gApp.touchSeen &&
            (CheckCollisionPointRec(mp, HudTouchPauseRect()) || CheckCollisionPointRec(mp, HudTouchAutoAimRect()));
        if (!onTouchUI) {
            Vector3 wp = GroundRaycast(g->camera, mp);
            VehicleSetTarget(v, wp);
        }
    }

    /* RMB: manual fire */
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        Vector3 wp = GroundRaycast(g->camera, GetMousePosition());
        /* Aim turret immediately */
        Vector3 d = Vector3Subtract(wp, v->pos);
        d.y = 0;
        float desired = -(atan2f(d.z, d.x) * RAD2DEG) + 90.0f;
        v->turretAngle = desired;
        TryFireTurret(g, wp);
    }

    /* Vehicle update */
    VehicleUpdate(v, &g->terrain, dt);

    /* Kinetic Bumper: damage enemies the moving vehicle touches (Nitro L3). */
    PartsRamPass(v, g->enemies, ENEMY_MAX, dt);

    /* Auto-aim: find nearest enemy and rotate turret */
    if (v->autoAim) {
        Enemy *nearest = EnemyFindNearest(g->enemies, ENEMY_MAX, v->pos, TURRET_RANGE);
        if (nearest) {
            VehicleAimTurretAt(v, nearest->pos, dt);
            /* Auto-fire when roughly aligned */
            Vector3 d = Vector3Subtract(nearest->pos, v->pos);
            d.y = 0;
            float desiredAngle = -(atan2f(d.z, d.x) * RAD2DEG) + 90.0f;
            float diff = fabsf(fmodf(v->turretAngle - desiredAngle + 540.0f, 360.0f) - 180.0f);
            if (diff < 8.0f) TryFireTurret(g, nearest->pos);
        }
    }

    /* HP regen is a future hook: apply per-second regen here. */

    /* Enemies */
    int newKills = 0;
    WaveUpdate(&g->wave, g->enemies, ENEMY_MAX, v, dt);
    EnemiesUpdate(g->enemies, ENEMY_MAX, v, g->projectiles, PROJ_MAX,
                  &g->terrain, dt, &newKills);
    g->kills += newKills;
    g->score += newKills * 100;
    if (newKills > 0 && gAudio) AudioPlayExplosion(gAudio);

    /* Projectiles */
    ProjectilesUpdate(g->projectiles, PROJ_MAX, g->enemies, ENEMY_MAX, dt);

    /* Scrap */
    CollectScrap(g);

    /* Camera follow */
    CameraFollow(&g->camera, v->pos, dt);

    /* Death check */
    if (v->hp <= 0) {
        g->state = STATE_GAMEOVER;
        if (gAudio) AudioPlayGameOver(gAudio);
        return;
    }

    /* Wave complete? */
    if (WaveIsComplete(&g->wave, g->enemies, ENEMY_MAX)) {
        g->state = STATE_RESPITE;
        g->wave.respiteTimer = WAVE_RESPITE_TIME;
        g->score += g->wave.number * 500;
    }
}

/* Main draw */
static void GameDraw(GameContext *g, SkyState *sky, PostFX *fx, const Settings *settings) {
    bool touch = (GetTouchPointCount() > 0) || gApp.touchSeen;

    /* 3D world rendered into the offscreen target */
    PostFXBegin(fx);

    /* Background (2D sky + sun, then far parallax rings/dust) before the scene. */
    SkyDrawBackground(sky, g->camera);

    BeginMode3D(g->camera);
    TerrainDraw(&g->terrain);
    VehicleDraw(&g->vehicle);
    EnemiesDraw(g->enemies, ENEMY_MAX, &g->camera);
    ProjectilesDraw(g->projectiles, PROJ_MAX);

    /* Move-target indicator */
    if (g->vehicle.moving) {
        DrawCircle3D(g->vehicle.target, 0.6f, (Vector3){1,0,0}, 90.0f,
                     (Color){220, 180, 60, 180});
    }
    EndMode3D();

    PostFXEnd(fx);

    /* Blit through shader, then 2D HUD on top (unshaded) */
    BeginDrawing();
    PostFXDraw(fx, GetFrameTime());

    if (gApp.screen == SCREEN_TITLE) {
        TitleDraw(gApp.runActive);
        if (gTitleSettings) {
            DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 0, 0, 0, 150 });
            SettingsPanelDraw(&gApp.settings);
            MenuButton((Rectangle){ SCREEN_W/2 - 150, SCREEN_H - 88, 300, 50 }, "BACK", true, false);
        }
    } else {
        switch (g->state) {
            case STATE_PLAYING:  HudDrawPlaying(g);  break;
            case STATE_RESPITE:  HudDrawPlaying(g);
                                 HudDrawRespite(g);  break;
            case STATE_UPGRADE:  HudDrawDraft(g);    break;
            case STATE_GAMEOVER: HudDrawGameOver(g); break;
            default: break;
        }
        if (g->state == STATE_PLAYING || g->state == STATE_RESPITE || g->state == STATE_UPGRADE)
            HudDrawRig(g);
        if (g->state == STATE_PLAYING) HudDrawTutorial(g, touch);
        if (gApp.touchSeen && g->state == STATE_PLAYING) HudDrawTouchControls(g);
        if (g->paused) PauseDraw(settings);
    }

    DrawVersion();
    DrawTransition();

    EndDrawing();

    InputEndFrame();
}
