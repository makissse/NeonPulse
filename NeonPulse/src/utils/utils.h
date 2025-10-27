#pragma once
#include "raylib.h"

// -------------------------
// Utility helpers
// -------------------------

bool RectsIntersect(const Rectangle& a, const Rectangle& b);
float Clamp1(float value, float min, float max);