#pragma once
#include "raylib.h"
#include <vector>
using namespace std;

// -------------------------
// Basic structs
// -------------------------
struct MovingPlatform {
    Rectangle base;
    float amplitude;
    float speed;
    bool vertical;
    Color color;
    float phase;

    Rectangle GetRect(float t) const;
};

struct Spike {
    Rectangle base;
    bool up;
    Color color;
};

struct Arch {
    Rectangle bounds;
    Color glow;
};

struct Section {
    float startX;
    float endX;
    Color bgA;
    Color bgB;
};

struct ParallaxLayer {
    float speed;
    Color color;
    int density;
    float scaleMin;
    float scaleMax;
};

struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float size;
    Color color;
};

struct JumpPad {
    Rectangle rect;
    float strength;
    Color color;
};

struct SpeedPad {
    Rectangle rect;
    float multiplier;
    float duration;
    Color color;
};

// -------------------------
// Functions
// -------------------------
bool CollideSpike(const Rectangle& player, const Spike& s);
void DrawSpike(const Spike& s, float camX);
