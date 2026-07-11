/*
 * Dust Runner - upgrade.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#pragma once
#include "game.h"

void UpgradePoolInit(void);
void UpgradePickChoices(UpgradeDef **choices, int count);
const UpgradeDef *UpgradeGetById(int id);
