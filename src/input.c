/*
 * Dust Runner - input.c
 * Copyright (c) 2026 Alvar Laigna  <https://alvarlaigna.itch.io/>
 */
#include "input.h"

static bool sWasTouching = false;

void InputBeginFrame(void) {
    /* sWasTouching holds the previous frame's touch state for edge detection. */
}

void InputEndFrame(void) {
    sWasTouching = GetTouchPointCount() > 0;
}

Vector2 InputPointerPosition(void) {
    if (GetTouchPointCount() > 0) return GetTouchPosition(0);
    return GetMousePosition();
}

bool InputPointerPressed(void) {
    bool touching = GetTouchPointCount() > 0;
    return (touching && !sWasTouching) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

bool InputPointerReleased(void) {
    bool touching = GetTouchPointCount() > 0;
    return (sWasTouching && !touching) || IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
}

bool InputPointerDown(void) {
    return GetTouchPointCount() > 0 || IsMouseButtonDown(MOUSE_BUTTON_LEFT);
}
