#include "entities.h"
#include "../utils/utils.h"
#include "raymath.h"
#include <cmath>

// -------------------------
// MovingPlatform
// -------------------------

Rectangle MovingPlatform::GetRect(float t) const {
    Rectangle r = base;
    float offset = amplitude * sinf(phase + t * speed * 2.0f * PI);
    if (vertical) r.y += offset;
    else r.x += offset;
    return r;
}

// -------------------------
// Spike collision
// -------------------------

bool CollideSpike(const Rectangle& player, const Spike& s) {
    float tipHeight = s.base.height * 0.72f;
    float tipWidth = s.base.width * 0.32f;
    Rectangle baseDanger = s.base;
    baseDanger.height *= 0.5f;
    if (!s.up) baseDanger.y = s.base.y + s.base.height * 0.5f;

    Rectangle tipBox;
    tipBox.width = tipWidth;
    tipBox.height = tipHeight;
    tipBox.x = s.base.x + (s.base.width - tipWidth) * 0.5f;
    tipBox.y = s.up ? (s.base.y - tipHeight + s.base.height) : s.base.y;

    return RectsIntersect(player, baseDanger) || RectsIntersect(player, tipBox);
}

// -------------------------
// Spike draw
// -------------------------

// Draw a spike (either pointing up from the floor, or pointing down from the ceiling)
void DrawSpike(const Spike& s, float camX) {
    BeginBlendMode(BLEND_ALPHA);

    Vector2 leftBase, rightBase, tip;

    if (s.up) {
        // Normal spikes (pointing upward)
        leftBase = { s.base.x - camX,                s.base.y + s.base.height };
        rightBase = { s.base.x + s.base.width - camX, s.base.y + s.base.height };
        tip = { s.base.x + s.base.width * 0.5f - camX, s.base.y };
    }
    else {
        // Ceiling spikes (pointing downward)
        // Fixed: reverse vertex order to correct winding (so fill is visible)
        rightBase = { s.base.x - camX,                s.base.y };
        leftBase = { s.base.x + s.base.width - camX, s.base.y };
        tip = { s.base.x + s.base.width * 0.5f - camX, s.base.y + s.base.height };
    }

    Color fillColor = s.color;
    Color outlineColor = { 0, 0, 0, 255 };

    // Draw filled triangle
    DrawTriangle(leftBase, rightBase, tip, fillColor);

    // Draw outline for better visibility
    DrawTriangleLines(leftBase, rightBase, tip, outlineColor);

    EndBlendMode();
}

