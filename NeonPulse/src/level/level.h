#pragma once

#include <vector>
#include "raylib.h"
#include "../entities/entities.h"

// Gravity pad type moved out of main.cpp
struct GravityPad
{
    Rectangle rect;  // rectangle area of the pad
    Color     color; // pad color
    bool      flipsUp; // true: flip gravity to inverted, false: back to normal
};

// This function will build the whole level layout:
// platforms, spikes, pads and finish line.
void BuildLevel(
    std::vector<MovingPlatform>& platforms,
    std::vector<GhostPlatform>& ghostPlatforms,
    std::vector<Spike>& spikes,
    std::vector<JumpPad>& jumpPads,
    std::vector<SpeedPad>& speedPads,
    std::vector<GravityPad>& gravityPads,
    Rectangle& finishLine,
    float defaultFloorY,
    float ceilingYTop,
    const Color& neonCyan,
    const Color& neonMagenta,
    const Color& neonYellow,
    const Color& neonGreen,
    const Color& neonBlue,
    const Color& neonPurple,
    const Color& neonRed
);
