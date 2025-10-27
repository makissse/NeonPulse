#include "utils.h"
#include <algorithm>
#include <cmath>

// -------------------------
// Utility helpers
// -------------------------

bool RectsIntersect(const Rectangle& a, const Rectangle& b) {
    return !(a.x + a.width <= b.x || b.x + b.width <= a.x ||
        a.y + a.height <= b.y || b.y + b.height <= a.y);
}

float Clamp1(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

