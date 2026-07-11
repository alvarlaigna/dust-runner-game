/*
 * Dust Runner - input.h
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 *
 * Unified pointer input: touch + mouse. Call InputBeginFrame once per frame
 * before queries, InputEndFrame once after all input handling.
 */
#ifndef INPUT_H
#define INPUT_H

#include "raylib.h"

void    InputBeginFrame(void);
void    InputEndFrame(void);
Vector2 InputPointerPosition(void);
bool    InputPointerPressed(void);
bool    InputPointerReleased(void);
bool    InputPointerDown(void);

#endif
