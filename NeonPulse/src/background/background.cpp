#include "background.h"
#include "../utils/utils.h"
#include <cmath>

using namespace std;


// -------------------------
// Background rendering implementation
// -------------------------

void DrawBackground(int screenW, int screenH, const Section& sec, float camX,
    const vector<ParallaxLayer>& layers, float beatPulse) {
    DrawRectangleGradientV(0, 0, screenW, screenH, sec.bgA, sec.bgB);

    int bandH = screenH / 8;
    Color bandColor = {
        (unsigned char)Clamp1(sec.bgB.r + (int)(28 * beatPulse), 0, 255),
        (unsigned char)Clamp1(sec.bgB.g + (int)(10 * beatPulse), 0, 255),
        (unsigned char)Clamp1(sec.bgB.b + (int)(36 * beatPulse), 0, 255),
        80
    };
    DrawRectangleGradientH(0, screenH / 2 - bandH / 2, screenW, bandH,
        Fade(bandColor, 0.08f), Fade(bandColor, 0.24f));

    for (const auto& layer : layers) {
        int count = layer.density;
        for (int i = 0; i < count; ++i) {
            float t = (float)i / (float)count;
            float x = fmodf(camX * layer.speed + t * 9000.0f, (float)screenW) - screenW * 0.5f + t * 140.0f;
            float y = (sinf(t * 12.1f) * 0.5f + 0.5f) * screenH;
            float size = layer.scaleMin + (layer.scaleMax - layer.scaleMin) * (0.5f + 0.5f * sinf(t * 7.9f));
            Color c = Fade(layer.color, 0.22f + 0.16f * beatPulse);

            if ((i + count) % 3 == 0) DrawCircle((int)x, (int)y, size, c);
            else {
                Vector2 center = { x, y };
                Vector2 a = { center.x, center.y - size };
                Vector2 b = { center.x + size, center.y };
                Vector2 d = { center.x - size, center.y };
                Vector2 e = { center.x, center.y + size };
                DrawTriangle(a, b, d, c);
                DrawTriangle(b, e, d, c);
            }
        }
    }
}
