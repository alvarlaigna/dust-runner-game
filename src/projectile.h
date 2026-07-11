/*
 * Dust Runner - projectile.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

void ProjectileFire(Projectile *pool, int maxCount, Vector3 origin,
                    Vector3 dir, const Vehicle *v);
void ProjectilesUpdate(Projectile *pool, int maxCount, Enemy *enemies,
                       int enemyMax, float dt);
void ProjectilesDraw(const Projectile *pool, int maxCount);
