/*
 * Dust Runner - enemy.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

void EnemySpawn(Enemy *pool, int maxCount, EnemyType type, Vector3 pos, int waveNum);
void EnemiesUpdate(Enemy *pool, int maxCount, Vehicle *v, Projectile *projPool,
                   int projMax, const Terrain *t, float dt, int *killCount);
void EnemiesDraw(const Enemy *pool, int maxCount, const Camera3D *cam);
int  EnemiesAliveCount(const Enemy *pool, int maxCount);
Enemy *EnemyFindNearest(Enemy *pool, int maxCount, Vector3 origin, float maxRange);
