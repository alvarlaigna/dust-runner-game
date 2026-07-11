/*
 * Dust Runner - vehicle.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

void VehicleInit(Vehicle *v);
void VehicleUpdate(Vehicle *v, const Terrain *t, float dt);
void VehicleDraw(const Vehicle *v);
void VehicleSetTarget(Vehicle *v, Vector3 target);
void VehicleApplyUpgrade(Vehicle *v, int upgradeId);
void VehicleAimTurretAt(Vehicle *v, Vector3 worldTarget, float dt);
