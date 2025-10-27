#pragma once
#include "raylib.h"
#include <vector>
#include "../entities/entities.h"

// -------------------------
// Background rendering
// -------------------------
// Draws the background with parallax layers and beat effects
// -------------------------

void DrawBackground(int screenW, int screenH, const Section& sec, float camX,
    const std::vector<ParallaxLayer>& layers, float beatPulse);
