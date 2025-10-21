#pragma once
#include "raylib.h"
#include <vector>
#include "../entities/entities.h"
using namespace std;

// -------------------------
// Background rendering
// -------------------------
void DrawBackground(int screenW, int screenH, const Section& sec, float camX,
    const vector<ParallaxLayer>& layers, float beatPulse);
