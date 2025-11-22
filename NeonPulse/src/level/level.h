#pragma once

#include <vector>
#include "raylib.h"
#include "../entities/entities.h"

// -------------------------
// Level building
// -------------------------

// This function is used to build the level by populating the provided vectors
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
