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
void DrawSpike(const Spike& s, float camX) {
    Vector2 p1 = { s.base.x - camX, s.base.y + s.base.height };
    Vector2 p2 = { s.base.x + s.base.width - camX, s.base.y + s.base.height };
    Vector2 p3;
    if (s.up) p3 = { s.base.x + s.base.width * 0.5f - camX, s.base.y };
    else      p3 = { s.base.x + s.base.width * 0.5f - camX, s.base.y + s.base.height };

    DrawTriangle(p1, p2, p3, s.color);
    DrawTriangleLines(p1, p2, p3, Fade(BLACK, 0.15f));
}
