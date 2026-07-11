/*
 * Dust Runner - vehicle.c
 * 3D box vehicle: smooth arrive steering, turret Y-axis rotation.
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "vehicle.h"
#include "terrain.h"
#include "rlgl.h"
#include <math.h>

void VehicleInit(Vehicle *v) {
    float mid = TERRAIN_WORLD * 0.5f;
    v->pos          = (Vector3){ mid, 0.0f, mid };
    v->target       = v->pos;
    v->angle        = 0.0f;
    v->turretAngle  = 0.0f;
    v->moving       = false;
    v->autoAim      = true;

    v->speed        = VEHICLE_SPEED_BASE;
    v->curSpeed     = 0.0f;
    v->steerAngle   = 0.0f;
    v->hp           = VEHICLE_HP_BASE;
    v->hpMax        = VEHICLE_HP_BASE;
    v->armor        = 0;
    v->ramDamage    = 5.0f;

    v->fireRate     = TURRET_FIRE_RATE;
    v->fireCooldown = 0.0f;
    v->damage       = PROJ_DAMAGE_BASE;
    v->projCount    = 1;
    v->hasChainLightning = false;
    v->hasFreeze    = false;
    v->hasExplosive = false;

    v->slowAuraRadius = 0.0f;
    v->slowAuraMod    = 1.0f;
    v->hasShield      = false;
    v->shieldCooldown = 0.0f;
    v->hasMagnet      = false;
    v->magnetRadius   = 0.0f;
}

/* Shortest-path angle difference, returns value in [-180, 180] */
static float AngleDiff(float from, float to) {
    float d = fmodf(to - from + 540.0f, 360.0f) - 180.0f;
    return d;
}

/* Rotate angle `from` toward `to` by at most `maxStep` degrees */
static float RotateToward(float from, float to, float maxStep) {
    float diff = AngleDiff(from, to);
    if (fabsf(diff) <= maxStep) return to;
    return from + (diff > 0 ? maxStep : -maxStep);
}

/* Forward unit vector for a body heading (inverse of the angle-from-direction
 * mapping used throughout: angle = -atan2(dz,dx)*RAD2DEG + 90). */
static Vector3 HeadingForward(float angleDeg) {
    float a = (90.0f - angleDeg) * DEG2RAD;
    return (Vector3){ cosf(a), 0.0f, sinf(a) };
}

/* Keep a world position inside the drivable bounds (graceful edge limit). */
static Vector3 ClampToBounds(Vector3 p) {
    float lo = VEHICLE_BOUND_MARGIN;
    float hi = TERRAIN_WORLD - VEHICLE_BOUND_MARGIN;
    p.x = Clamp(p.x, lo, hi);
    p.z = Clamp(p.z, lo, hi);
    return p;
}

void VehicleUpdate(Vehicle *v, const Terrain *t, float dt) {
    /* Truck-like steering (bicycle model): drives along its heading; the wheel
     * angle slews the heading toward the target. Momentum + wheelbase turn
     * radius + speed understeer give it weight. */
    Vector3 toTarget = Vector3Subtract(v->target, v->pos);
    toTarget.y = 0.0f;
    float dist = Vector3Length(toTarget);

    float maxSpeed = v->speed * TerrainSpeedMod(t, v->pos);

    /* Desired speed: 0 at the target, eased down on approach (smooth arrival). */
    float targetSpeed = (dist <= VEHICLE_ARRIVE_DIST)
                      ? 0.0f
                      : maxSpeed * fminf(1.0f, dist / 6.0f);

    if (dist > VEHICLE_ARRIVE_DIST) {
        /* Steer toward the heading that points at the target. */
        float desired = -(atan2f(toTarget.z, toTarget.x) * RAD2DEG) + 90.0f;
        float err = AngleDiff(v->angle, desired);            /* [-180, 180] */

        /* Speed-dependent max steer: understeer at high speed. */
        float speedFrac = (v->speed > 0.0f) ? (v->curSpeed / v->speed) : 0.0f;
        float maxSteer  = VEHICLE_MAX_STEER * (1.0f - 0.45f * fminf(1.0f, speedFrac));
        float wantSteer = Clamp(err, -maxSteer, maxSteer);
        v->steerAngle   = RotateToward(v->steerAngle, wantSteer, VEHICLE_STEER_RATE * dt);

        /* Ease off the throttle for sharp turns. */
        targetSpeed *= 1.0f - 0.5f * fminf(1.0f, fabsf(err) / 90.0f);

        /* Low-speed pivot assist: when nearly stopped but badly mis-aimed (e.g.
         * the target is behind), rotate in place so we never circle endlessly or
         * get wedged against an edge. */
        if (v->curSpeed < 4.0f && fabsf(err) > 55.0f) {
            v->angle = RotateToward(v->angle, desired, 70.0f * dt);
        }
    } else {
        /* Straighten the wheel at rest. */
        v->steerAngle = RotateToward(v->steerAngle, 0.0f, VEHICLE_STEER_RATE * dt);
    }

    /* Accelerate / brake toward the desired speed (momentum). */
    float rate = (targetSpeed > v->curSpeed) ? VEHICLE_ACCEL : VEHICLE_BRAKE;
    float ds   = rate * dt;
    if (fabsf(targetSpeed - v->curSpeed) <= ds) v->curSpeed = targetSpeed;
    else v->curSpeed += (targetSpeed > v->curSpeed) ? ds : -ds;
    if (v->curSpeed < 0.0f) v->curSpeed = 0.0f;

    /* Rotate heading via the bicycle model: dTheta = (v / L) * tan(steer). */
    if (v->curSpeed > 0.01f) {
        float dTheta = (v->curSpeed / VEHICLE_WHEELBASE)
                     * tanf(v->steerAngle * DEG2RAD) * dt * RAD2DEG;
        v->angle += dTheta;
        if (v->angle >= 360.0f) v->angle -= 360.0f;
        if (v->angle < 0.0f)    v->angle += 360.0f;
    }

    /* Advance along the heading; clamp to bounds (soft edge) and stop on ruins. */
    Vector3 next = Vector3Add(v->pos, Vector3Scale(HeadingForward(v->angle), v->curSpeed * dt));
    next = ClampToBounds(next);            /* graceful edge: slide along, never leave */
    if (TerrainIsPassable(t, next)) {
        v->pos = next;
    } else {
        /* Ruin ahead: bleed speed and hold position; the pivot assist above lets
         * the truck turn away rather than getting stuck. */
        v->curSpeed *= 0.4f;
        v->target = ClampToBounds(v->target);
    }
    v->moving = (v->curSpeed > 0.05f);

    /* Cooldowns */
    if (v->fireCooldown > 0.0f) v->fireCooldown -= dt;
    if (v->shieldCooldown > 0.0f) v->shieldCooldown -= dt;
}

/* Set turret angle toward a world-space target (called from main/enemy system) */
void VehicleAimTurretAt(Vehicle *v, Vector3 worldTarget, float dt) {
    Vector3 d = Vector3Subtract(worldTarget, v->pos);
    float desired = -(atan2f(d.z, d.x) * RAD2DEG) + 90.0f;
    v->turretAngle = RotateToward(v->turretAngle, desired, TURRET_ROT_SPEED * dt);
}

void VehicleSetTarget(Vehicle *v, Vector3 target) {
    /* Clamp commands into bounds so the player can't drive into the edge. */
    v->target = ClampToBounds(target);
}

/* Drawing */
void VehicleDraw(const Vehicle *v) {
    /* Vehicle drawn as coloured boxes - no external assets needed */

    /* Chassis */
    rlPushMatrix();
    rlTranslatef(v->pos.x, 0.0f, v->pos.z);
    /* Body is modelled with its front along local +X; rotating by `angle`
     * alone maps +X 90° off the heading, so the truck drove sideways. The -90°
     * offset aligns the model's front with HeadingForward(angle). The turret is
     * drawn relative to the body, so it stays correct too. */
    rlRotatef(v->angle - 90.0f, 0, 1, 0);

    /* Main hull */
    DrawCube((Vector3){0, 0.55f, 0},  3.2f, 0.8f, 1.8f, (Color){60, 100, 60, 255});
    /* Cab */
    DrawCube((Vector3){0.4f, 1.15f, 0}, 1.4f, 0.6f, 1.4f, (Color){50, 85, 50, 255});
    /* Wheels: rear pair fixed, front pair steers with the wheel angle. */
    Color wheelCol = (Color){30, 30, 30, 255};
    DrawCube((Vector3){-1.2f, 0.22f,  1.0f}, 0.7f, 0.44f, 0.5f, wheelCol);
    DrawCube((Vector3){-1.2f, 0.22f, -1.0f}, 0.7f, 0.44f, 0.5f, wheelCol);
    for (int sgn = -1; sgn <= 1; sgn += 2) {
        rlPushMatrix();
        rlTranslatef(1.2f, 0.22f, sgn * 1.0f);
        rlRotatef(v->steerAngle, 0, 1, 0);
        DrawCube((Vector3){0, 0, 0}, 0.7f, 0.44f, 0.5f, wheelCol);
        rlPopMatrix();
    }

    /* Turret base */
    DrawCylinder((Vector3){0, 1.1f, 0}, 0.45f, 0.45f, 0.35f, 8, (Color){80, 80, 80, 255});

    /* Turret barrel - rotated around Y by turretAngle relative to body */
    rlPushMatrix();
    rlRotatef(v->turretAngle - v->angle, 0, 1, 0);
    DrawCube((Vector3){0.9f, 1.28f, 0}, 1.4f, 0.18f, 0.18f, (Color){100, 100, 100, 255});
    rlPopMatrix();

    rlPopMatrix();

    /* HP bar (world-space, above vehicle) */
    float hpFrac = (float)v->hp / (float)v->hpMax;
    Vector3 barCenter = { v->pos.x, 2.5f, v->pos.z };
    DrawCube(barCenter, 2.0f, 0.15f, 0.05f, (Color){60, 60, 60, 200});
    Vector3 barFill = { v->pos.x - 1.0f + hpFrac, 2.5f, v->pos.z };
    DrawCube(barFill, 2.0f * hpFrac, 0.15f, 0.06f,
             hpFrac > 0.5f ? (Color){50, 200, 50, 255} :
             hpFrac > 0.25f ? (Color){220, 180, 30, 255} :
                              (Color){220, 50, 50, 255});
}

void VehicleApplyUpgrade(Vehicle *v, int upgradeId) {
    switch (upgradeId) {
        /* Weapon */
        case 0:  v->fireRate  *= 1.30f; break;
        case 1:  v->damage    += 10;    break;
        case 2:  v->projCount++;        break;
        case 3:  v->hasFreeze    = true; break;
        case 4:  v->hasExplosive = true; break;
        case 5:  v->hasChainLightning = true; break;
        /* Chassis */
        case 10: v->speed     *= 1.20f; break;
        case 11: v->armor     += 5;     break;
        case 12: v->ramDamage *= 1.50f; break;
        case 13: v->hpMax     += 30; v->hp = v->hpMax; break;
        /* Utility */
        case 20: v->slowAuraRadius = 8.0f; v->slowAuraMod = 0.6f; break;
        case 21: v->hasMagnet = true; v->magnetRadius = 12.0f; break;
        case 22: v->hasShield = true; break;
        default: break;
    }
}
