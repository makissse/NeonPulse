#include "utils.h"
#include <algorithm>
#include <cmath>

using namespace std;

// -------------------------
// Utility helpers
// -------------------------


// Check if two rectangles intersect
bool RectsIntersect(const Rectangle& a, const Rectangle& b) {
    return !(a.x + a.width <= b.x || b.x + b.width <= a.x ||
        a.y + a.height <= b.y || b.y + b.height <= a.y);
}

// Clamp a float value between min and max
float Clamp1(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

