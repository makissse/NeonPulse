
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>

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

    Rectangle GetRect(float t) const {
        Rectangle r = base;
        float offset = amplitude * sinf(phase + t * speed * 2.0f * PI);
        if (vertical) r.y += offset;
        else r.x += offset;
        return r;
    }
};

struct Spike {
    Rectangle base;
    bool up;
    Color color;
};

struct Arch {             // decorative arch (was tunnel) - doesn't change floor/ceiling
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

// New gameplay items
struct JumpPad {
    Rectangle rect;
    float strength;   // multiplies jumpVel (positive upward)
    Color color;
};

struct SpeedPad {
    Rectangle rect;
    float multiplier; // multiplies runSpeed for a short duration
    float duration;   // seconds
    Color color;
};

// -------------------------
// Helpers
// -------------------------
static bool RectsIntersect(const Rectangle& a, const Rectangle& b) {
    return !(a.x + a.width <= b.x || b.x + b.width <= a.x ||
        a.y + a.height <= b.y || b.y + b.height <= a.y);
}

static bool CollideSpike(const Rectangle& player, const Spike& s) {
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

static void DrawSpike(const Spike& s, float camX) {
    Vector2 p1 = { s.base.x - camX, s.base.y + s.base.height };
    Vector2 p2 = { s.base.x + s.base.width - camX, s.base.y + s.base.height };
    Vector2 p3;
    if (s.up) p3 = { s.base.x + s.base.width * 0.5f - camX, s.base.y };
    else      p3 = { s.base.x + s.base.width * 0.5f - camX, s.base.y + s.base.height };

    DrawTriangle(p1, p2, p3, s.color);
    DrawTriangleLines(p1, p2, p3, Fade(BLACK, 0.15f));
}

// Background draw
static void DrawBackground(int screenW, int screenH, const Section& sec, float camX,
    const vector<ParallaxLayer>& layers, float beatPulse) {
    DrawRectangleGradientV(0, 0, screenW, screenH, sec.bgA, sec.bgB);

    int bandH = screenH / 8;
    Color bandColor = { (unsigned char)Clamp(sec.bgB.r + (int)(28 * beatPulse), 0, 255),
                        (unsigned char)Clamp(sec.bgB.g + (int)(10 * beatPulse), 0, 255),
                        (unsigned char)Clamp(sec.bgB.b + (int)(36 * beatPulse), 0, 255),
                        80 };
    DrawRectangleGradientH(0, screenH / 2 - bandH / 2, screenW, bandH, Fade(bandColor, 0.08f), Fade(bandColor, 0.24f));

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

// -------------------------
// Level design parameters
// -------------------------
int main() {
    const int screenW = 1280;
    const int screenH = 720;
    InitWindow(screenW, screenH, "Neon Pulse - Level Redesign (raylib)");
    SetTargetFPS(120);

    // Rhythm
    const float BPM = 140.0f;
    const float secondsPerBeat = 60.0f / BPM;
    float songTime = 0.0f;

    // Player
    Rectangle player{ 100, 520, 36, 36 };
    Vector2 playerVel{ 0.0f, 0.0f };
    const float baseRunSpeedDefault = 420.0f;
    float baseRunSpeed = baseRunSpeedDefault;
    float runSpeed = baseRunSpeed;
    const float gravity = 1800.0f;
    const float jumpVel = -760.0f;
    bool grounded = false;
    bool alive = true;
    float deathShake = 0.0f;

    // SpeedPad state
    float speedTimer = 0.0f;
    float speedMultiplierActive = 1.0f;

    // Camera
    float camX = 0.0f;

    // Floor & ceiling
    const float defaultFloorY = 560.0f;
    const float ceilingYTop = 80.0f;

    // Colors
    Color neonCyan = { 0, 255, 255, 255 };
    Color neonMagenta = { 255, 0, 200, 255 };
    Color neonYellow = { 255, 240, 0, 255 };
    Color neonGreen = { 50, 255, 160, 255 };
    Color neonBlue = { 60, 160, 255, 255 };
    Color neonPurple = { 170, 60, 255, 255 };

    // Sections (visual)
    vector<Section> sections = {
        { 0.0f,     1200.0f,  { 20, 30, 60, 255 }, { 40, 10, 80, 255 } },
        { 1200.0f,  2600.0f,  { 10, 50, 80, 255 }, { 0, 20, 40, 255 } },
        { 2600.0f,  4200.0f,  { 10, 10, 40, 255 }, { 40, 0, 60, 255 } },
        { 4200.0f,  7600.0f,  { 8, 12, 26, 255  }, { 18, 26, 64, 255 } },
    };

    // Parallax layers
    vector<ParallaxLayer> layers = {
        { 0.06f, neonBlue,   16, 10.0f, 30.0f },
        { 0.12f, neonPurple, 20, 6.0f,  20.0f },
        { 0.22f, neonCyan,   28, 4.0f,  14.0f },
    };

    // Moving platforms: redesigned sequences with rhythm
    vector<MovingPlatform> platforms;

    // ---- Intro platforms (easy, short gaps) ----
    platforms.push_back({ { 480,  defaultFloorY - 80, 160, 20 }, 0, 0.0f, false, neonGreen, 0.0f }); // static small
    platforms.push_back({ { 800,  defaultFloorY - 110, 160, 20 }, 40, 1.0f, true, neonYellow, 0.0f }); // vertical
    platforms.push_back({ { 1080, defaultFloorY - 160, 140, 20 }, 0, 0.0f, false, neonMagenta, 0.0f }); // static

    // ---- Rhythm hop section (player times jumps to beat) ----
    // small platforms arranged at beat intervals
    float beatGap = 220.0f; // horizontal spacing per beat
    float startBeatX = 1400.0f;
    for (int i = 0; i < 6; ++i) {
        platforms.push_back({ { startBeatX + i * beatGap, defaultFloorY - 140.0f - ((i % 2) * 12.0f), 120, 20 }, 0, 0.0f, false, (i % 2 ? neonCyan : neonPurple), (float)i * 0.25f });
    }

    // ---- Moving platform chain (requires timing) ----
    platforms.push_back({ { 2600, defaultFloorY - 200, 160, 20 }, 90, 0.9f, true, neonBlue, 0.2f });
    platforms.push_back({ { 2920, defaultFloorY - 220, 150, 20 }, 120, 1.1f, false, neonMagenta, 1.1f });
    platforms.push_back({ { 3250, defaultFloorY - 180, 180, 20 }, 70, 1.2f, true, neonYellow, 0.5f });
    platforms.push_back({ { 3550, defaultFloorY - 210, 140, 20 }, 90, 0.9f, false, neonGreen, 0.8f });

    // ---- Long jump challenge ----
    platforms.push_back({ { 4200, defaultFloorY - 140, 140, 20 }, 0, 0.0f, false, neonCyan, 0.0f });
    // gap, then landing platform
    platforms.push_back({ { 4700, defaultFloorY - 150, 180, 20 }, 0, 0.0f, false, neonMagenta, 0.0f });

    // ---- Final ascending area (stairs + moving) ----
    for (int i = 0; i < 6; ++i) {
        platforms.push_back({
        { 5400 + i * 140.0f, defaultFloorY - 100.0f - i * 36.0f, 110, 20 },
        (i % 2 ? 40.0f : 60.0f),            // <Ч добавил .0f
        0.9f + i * 0.05f,
        (i % 2 == 0),
        (i % 2 ? neonYellow : neonPurple),
        i * 0.3f
            });

    }

    // Finish line (visual) - not a platform, just marker later.

    // Spikes: redesigned to match sections (clear readable patterns)
    vector<Spike> spikes;
    auto addSpikeCluster = [&](float startX, int count, float w, float h, bool up, Color c) {
        for (int i = 0; i < count; i++) {
            float x = startX + i * (w * 0.86f);
            float y = up ? (defaultFloorY - h) : (ceilingYTop);
            spikes.push_back({ { x, y, w, h }, up, c });
        }
        };

    // Intro: small triple
    addSpikeCluster(640.0f, 3, 40.0f, 60.0f, true, neonYellow);

    // Rhythm section: spikes between beat platforms but spaced so jump timing matters
    addSpikeCluster(1600.0f, 4, 36.0f, 60.0f, true, neonMagenta);

    // Moving chain: ceiling spikes require duck (if existed) Ч but we keep them as risky overhangs
    addSpikeCluster(3020.0f, 3, 38.0f, 60.0f, false, neonPurple);

    // Long jump approach: spike pit to emphasize long jump
    addSpikeCluster(4360.0f, 6, 40.0f, 70.0f, true, neonCyan);

    // Final: alternating spikes to drive challenge
    addSpikeCluster(5650.0f, 8, 36.0f, 60.0f, true, neonGreen);

    // Decorative arches (previously tunnels) - purely visual, do NOT modify floor/ceiling
    vector<Arch> arches = {
        { { 1500, ceilingYTop - 10, 420, defaultFloorY - ceilingYTop + 10 }, neonCyan },
        { { 3000, ceilingYTop - 8, 520, defaultFloorY - ceilingYTop + 8 }, neonMagenta },
        { { 5200, ceilingYTop - 12, 700, defaultFloorY - ceilingYTop + 12 }, neonYellow }
    };

    // JumpPads & SpeedPads
    vector<JumpPad> jumpPads;
    vector<SpeedPad> speedPads;

    // Place a jump pad before the long jump to guarantee the jump
    jumpPads.push_back({ { 4080, defaultFloorY - 32, 64, 16 }, 1.65f, neonYellow });  // bigger bounce

    // Speed pads: in rhythm section to change feel
    speedPads.push_back({ { 1480, defaultFloorY - 8, 60, 8 }, 1.2f, 1.2f, neonGreen });
    speedPads.push_back({ { 2200, defaultFloorY - 8, 60, 8 }, 1.35f, 0.9f, neonCyan });
    speedPads.push_back({ { 5000, defaultFloorY - 8, 60, 8 }, 1.25f, 1.0f, neonMagenta });

    // Particle container
    vector<Particle> particles;
    particles.reserve(400);

    // Beat pulse function
    auto BeatPulse = [&](float t) {
        float beat = fmodf(t, secondsPerBeat);
        return expf(-6.0f * beat);
        };

    // Helper: current section
    auto CurrentSection = [&](float x) -> Section {
        for (auto& s : sections) {
            if (x >= s.startX && x < s.endX) return s;
        }
        return sections.back();
        };

    // Finish zone
    Rectangle finishLine = { 6800.0f, 0.0f, 8.0f, (float)screenH };

    // Main loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        songTime += dt;

        // Restart if dead
        if (!alive && IsKeyPressed(KEY_R)) {
            player = { 100, 520, 36, 36 };
            playerVel = { 0.0f, 0.0f };
            grounded = false;
            alive = true;
            camX = 0.0f;
            songTime = 0.0f;
            deathShake = 0.0f;
            particles.clear();
            runSpeed = baseRunSpeedDefault;
            baseRunSpeed = baseRunSpeedDefault;
            speedTimer = 0.0f;
            speedMultiplierActive = 1.0f;
        }

        // Input: jump
        if (alive) {
            if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP)) && grounded) {
                playerVel.y = jumpVel;
                grounded = false;
                // jump particles
                for (int i = 0; i < 10; ++i) {
                    float ang = (float)GetRandomValue(-80, -100) * DEG2RAD;
                    float sp = (float)GetRandomValue(160, 320);
                    particles.push_back({ { player.x + player.width * 0.5f, player.y + player.height }, { cosf(ang) * sp, sinf(ang) * sp * -1.0f }, 0.45f + GetRandomValue(0,20) * 0.01f, (float)GetRandomValue(2,6), Fade(neonYellow, 0.9f) });
                }
            }
        }

        // reset base runSpeed if no active speedpad
        if (speedTimer <= 0.0f) {
            speedMultiplierActive = 1.0f;
            runSpeed = baseRunSpeed;
        }
        else {
            speedTimer -= dt;
            if (speedTimer <= 0.0f) {
                speedTimer = 0.0f;
                speedMultiplierActive = 1.0f;
                runSpeed = baseRunSpeed;
            }
            else {
                runSpeed = baseRunSpeed * speedMultiplierActive;
            }
        }

        // player horizontal control (auto-run)
        if (alive) playerVel.x = runSpeed;
        else playerVel.x = 0.0f;

        // gravity
        playerVel.y += gravity * dt;

        // integrate
        if (alive) {
            player.x += playerVel.x * dt;
            player.y += playerVel.y * dt;
        }

        // floor collision (no tunnels override now)
        grounded = false;
        if (player.y + player.height >= defaultFloorY) {
            player.y = defaultFloorY - player.height;
            playerVel.y = 0.0f;
            grounded = true;
        }
        // ceiling clamp
        if (player.y <= ceilingYTop) {
            player.y = ceilingYTop;
            if (playerVel.y < 0.0f) playerVel.y = 0.0f;
        }

        // Moving platforms collision + resolve
        float pulse = BeatPulse(songTime);
        float tPhase = songTime + pulse * 0.03f;
        for (auto& p : platforms) {
            Rectangle pr = p.GetRect(tPhase);
            if (pr.x + pr.width < player.x - 300.0f || pr.x > player.x + 900.0f) continue; // cull
            if (RectsIntersect(player, pr)) {
                Rectangle prevPlayer = { player.x - playerVel.x * dt, player.y - playerVel.y * dt, player.width, player.height };

                bool fromTop = (prevPlayer.y + prevPlayer.height <= pr.y + 1.0f);
                bool fromBottom = (prevPlayer.y >= pr.y + pr.height - 1.0f);
                bool fromLeft = (prevPlayer.x + prevPlayer.width <= pr.x + 1.0f);
                bool fromRight = (prevPlayer.x >= pr.x + pr.width - 1.0f);

                if (fromTop) {
                    player.y = pr.y - player.height;
                    playerVel.y = 0.0f;
                    grounded = true;
                    // If platform moves horizontally, carry a fraction
                    if (!p.vertical) {
                        float platformVel = cosf(p.phase + tPhase * p.speed * 2.0f * PI) * p.amplitude * p.speed;
                        player.x += platformVel * dt * 0.08f; // small carry
                    }
                }
                else if (fromBottom) {
                    player.y = pr.y + pr.height;
                    playerVel.y = 0.0f;
                }
                else if (fromLeft) {
                    player.x = pr.x - player.width;
                }
                else if (fromRight) {
                    player.x = pr.x + pr.width;
                }
            }
        }

        // JumpPad activation
        for (const auto& jp : jumpPads) {
            if (RectsIntersect(player, jp.rect)) {
                playerVel.y = jumpVel * jp.strength; // immediately boost up
                grounded = false;
                // Jump pad particles
                for (int i = 0; i < 16; ++i) {
                    float ang = (float)GetRandomValue(-110, -70) * DEG2RAD;
                    float sp = (float)GetRandomValue(220, 420);
                    particles.push_back({ { player.x + player.width * 0.5f, player.y + player.height }, { cosf(ang) * sp, sinf(ang) * sp * -1.0f }, 0.5f + GetRandomValue(0,20) * 0.01f, (float)GetRandomValue(3,7), Fade(jp.color, 0.95f) });
                }
            }
        }

        // SpeedPad activation (instant apply multiplier)
        for (const auto& sp : speedPads) {
            if (RectsIntersect(player, sp.rect)) {
                speedTimer = sp.duration;
                speedMultiplierActive = sp.multiplier;
                runSpeed = baseRunSpeed * speedMultiplierActive;
                // speed particles
                for (int i = 0; i < 12; ++i) {
                    float ang = (float)GetRandomValue(-20, 20) * DEG2RAD;
                    float spv = (float)GetRandomValue(80, 260);
                    particles.push_back({ { player.x + player.width * 0.5f, player.y + player.height * 0.5f }, { cosf(ang) * spv, sinf(ang) * spv }, 0.35f + GetRandomValue(0,10) * 0.01f, (float)GetRandomValue(2,4), Fade(sp.color, 0.9f) });
                }
            }
        }

        // Spike collision -> death
        for (const auto& s : spikes) {
            if (player.x + player.width > s.base.x - 20 && player.x < s.base.x + s.base.width + 20) {
                if (CollideSpike(player, s)) {
                    alive = false;
                    deathShake = 8.0f;
                    break;
                }
            }
        }

        // Camera follows player
        camX = player.x - 280.0f;

        // Particles update & cleanup
        for (int i = (int)particles.size() - 1; i >= 0; --i) {
            particles[i].life -= dt;
            if (particles[i].life <= 0.0f) {
                particles.erase(particles.begin() + i);
                continue;
            }
            particles[i].pos.x += particles[i].vel.x * dt;
            particles[i].pos.y += particles[i].vel.y * dt;
            particles[i].vel.x *= (1.0f - 3.0f * dt);
            particles[i].vel.y += 500.0f * dt;
        }

        if (deathShake > 0.0f) deathShake = max(0.0f, deathShake - 24.0f * dt);

        // === RENDER ===
        BeginDrawing();
        ClearBackground(BLACK);

        float shakeX = (GetRandomValue(-1000, 1000) / 1000.0f) * deathShake;
        float shakeY = (GetRandomValue(-1000, 1000) / 1000.0f) * deathShake;

        Section sec = CurrentSection(camX + screenW * 0.5f);
        DrawBackground(screenW, screenH, sec, camX, layers, pulse);

        // Decorative arches (pure visuals)
        for (const auto& a : arches) {
            float x = a.bounds.x - camX;
            if (x + a.bounds.width < -120 || x > screenW + 120) continue;
            Color glow = Fade(a.glow, 0.30f + 0.18f * pulse);
            DrawRectangle((int)(x), (int)(ceilingYTop - 8), (int)a.bounds.width, 8, glow);
            DrawRectangle((int)(x + a.bounds.width * 0.1f), (int)(defaultFloorY), (int)(a.bounds.width * 0.8f), 6, Fade(glow, 0.12f));
            // vertical side bars
            DrawRectangle((int)x, (int)(ceilingYTop), 6, (int)(defaultFloorY - ceilingYTop), Fade(glow, 0.5f));
            DrawRectangle((int)(x + a.bounds.width - 6), (int)(ceilingYTop), 6, (int)(defaultFloorY - ceilingYTop), Fade(glow, 0.5f));
        }

        // Floor and ceiling rails
        Color railA = Fade(neonBlue, 0.45f + 0.2f * pulse);
        Color railB = Fade(neonPurple, 0.45f + 0.2f * pulse);
        DrawRectangleGradientH(0, (int)defaultFloorY, screenW, 6, railA, railB);
        DrawRectangleGradientH(0, (int)ceilingYTop - 6, screenW, 6, railB, railA);

        // Draw speed pads & jump pads
        for (const auto& sp : speedPads) {
            float x = sp.rect.x - camX;
            if (x + sp.rect.width < -120 || x > screenW + 120) continue;
            DrawRectangle((int)(x), (int)(sp.rect.y), (int)sp.rect.width, (int)sp.rect.height, Fade(sp.color, 0.95f));
            DrawRectangleLinesEx({ x, sp.rect.y, sp.rect.width, sp.rect.height }, 2.0f, Fade(WHITE, 0.06f));
        }
        for (const auto& jp : jumpPads) {
            float x = jp.rect.x - camX;
            if (x + jp.rect.width < -120 || x > screenW + 120) continue;
            DrawRectangleRounded({ x, jp.rect.y, jp.rect.width, jp.rect.height }, 0.3f, 6, Fade(jp.color, 0.95f));
            DrawRectangleLinesEx({ x, jp.rect.y, jp.rect.width, jp.rect.height }, 2.0f, Fade(WHITE, 0.06f));
        }

        // Moving platforms
        for (const auto& p : platforms) {
            Rectangle r = p.GetRect(tPhase);
            if (r.x + r.width - camX < -160 || r.x - camX > screenW + 160) continue;
            Rectangle drawR = { r.x - camX + shakeX, r.y + shakeY, r.width, r.height };
            Color fill = Fade(p.color, 0.45f + 0.28f * pulse);
            Color edge = Fade(p.color, 0.96f);
            DrawRectangleRounded(drawR, 0.18f, 6, fill);
            DrawRectangleLinesEx(drawR, 3.0f, edge);
            DrawRectangle((int)drawR.x, (int)(drawR.y + drawR.height), (int)drawR.width, 6, Fade(p.color, 0.28f));
        }

        // Spikes
        for (const auto& s : spikes) {
            float x = s.base.x - camX;
            if (x + s.base.width < -160 || x > screenW + 160) continue;
            DrawSpike(s, camX);
        }

        // Particles behind player
        for (const auto& prt : particles) {
            Vector2 ppos = { prt.pos.x - camX + shakeX, prt.pos.y + shakeY };
            DrawCircleV(ppos, prt.size, Fade(prt.color, Clamp(prt.life * 2.5f, 0.0f, 1.0f)));
        }

        // Player draw
        Rectangle drawPlayer = { player.x - camX + shakeX, player.y + shakeY, player.width, player.height };
        Color playerFill = Fade(neonCyan, alive ? 0.92f : 0.28f);
        Color playerEdge = Fade(neonMagenta, alive ? 1.0f : 0.45f);
        DrawRectangleRounded(drawPlayer, 0.18f, 8, playerFill);
        DrawRectangleLinesEx(drawPlayer, 3.0f, playerEdge);
        DrawRectangle((int)(drawPlayer.x - 6), (int)(drawPlayer.y - 6), (int)(drawPlayer.width + 12), (int)(drawPlayer.height + 12), Fade(neonCyan, 0.03f + 0.05f * pulse));

        // Beat ring
        float ringR = 22.0f + 18.0f * pulse;
        DrawCircleLines((int)(drawPlayer.x + drawPlayer.width * 0.5f), (int)(drawPlayer.y + drawPlayer.height * 0.5f),
            ringR, Fade(neonYellow, 0.6f * pulse));

        // Finish line visual
        if (finishLine.x - camX < screenW + 200) {
            DrawRectangle((int)(finishLine.x - camX), 0, 4, screenH, Fade(neonGreen, 0.95f));
            DrawText("FINISH", (int)(finishLine.x - camX) - 30, screenH / 2 - 12, 20, Fade(WHITE, 0.9f));
        }

        // HUD
        DrawText("Neon Pulse - Redesign", 24, 20, 28, Fade(WHITE, 0.9f));
        DrawText(TextFormat("BPM: %.0f", BPM), 24, 56, 20, Fade(WHITE, 0.6f));
        DrawText("Jump: Space/Up | Restart: R", 24, 84, 18, Fade(WHITE, 0.6f));
        if (speedTimer > 0.0f) {
            DrawText(TextFormat("SPEED x%.2f (%.1fs)", speedMultiplierActive, speedTimer), 24, 108, 18, Fade(neonGreen, 0.9f));
        }

        if (!alive) {
            const char* msg = "Crashed! Press R to restart";
            int fw = MeasureText(msg, 40);
            DrawText(msg, screenW / 2 - fw / 2, screenH / 3, 40, Fade(RED, 0.95f));
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
