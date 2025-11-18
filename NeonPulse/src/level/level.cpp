#include "level.h"
#include "raymath.h"   // if you need it here
#include <cmath>
#include <cstdlib>
#include <algorithm>

// Local helper to add spike clusters (copied from main.cpp)
static void AddSpikeCluster(
    std::vector<Spike>& spikes,
    float startX,
    int count,
    float w,
    float h,
    bool up,
    Color c,
    float defaultFloorY,
    float ceilingYTop
) {
    // This helper spawns a row of spikes either on floor or on ceiling
    for (int i = 0; i < count; i++) {
        float x = startX + i * (w * 0.86f);
        float y = up ? (defaultFloorY - h) : (ceilingYTop);
        spikes.push_back({ { x, y, w, h }, up, c });
    }
}

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
)
{
    // --- Intro: tutorial ---
    {
        AddSpikeCluster(spikes, 900.0f, 1, 36.0f, 56.0f, true, neonYellow, defaultFloorY, ceilingYTop);
        AddSpikeCluster(spikes, 1300.0f, 2, 36.0f, 56.0f, true, neonYellow, defaultFloorY, ceilingYTop);
    }

    // --- Easy rhythm (small hops) ---
    {
        platforms.push_back({ { 1780,  defaultFloorY - 72, 140, 20 }, 0, 0.0f, false, neonGreen, 0.0f });
        platforms.push_back({ { 2060,  defaultFloorY - 84, 140, 20 }, 0, 0.0f, false, neonCyan,  0.0f });
        platforms.push_back({ { 2340, defaultFloorY - 100, 140, 20 }, 0, 0.0f, false, neonMagenta, 0.0f });

        AddSpikeCluster(spikes, 2200.0f, 2, 36.0f, 56.0f, true, neonYellow, defaultFloorY, ceilingYTop);
    }

    // --- Beat Hop: consistent spacing, one intended path ---
    {
        float beatGap = 180.0f;
        float beatStart = 2620.0f;

        for (int i = 0; i < 8; ++i) {
            float yOff = (i % 2 == 0) ? -128.0f : -140.0f;
            if (i % 2 == 0) {
                platforms.push_back({
                    { beatStart + i * beatGap, defaultFloorY + yOff, 110, 18 },
                    0.0f, 0.0f, false, neonBlue, 0.0f
                    });
            }
            else {
                ghostPlatforms.push_back({
                    { beatStart + i * beatGap, defaultFloorY + yOff, 110, 18 },
                    0.0f, 0.0f, false, neonPurple, 0.0f
                    });
            }
        }

        for (int i = 0; i < 8; ++i) {
            float gapCenterX = beatStart + i * beatGap - beatGap * 0.5f;
            AddSpikeCluster(spikes, gapCenterX - 16.0f, 6, 34.0f, 60.0f, true,
                neonMagenta, defaultFloorY, ceilingYTop);
        }
    }

    // --- Speedlaunch (short boost into a simple chain) ---
    {
        speedPads.push_back({ { 4100, defaultFloorY - 8, 66, 8 }, 1.35f, 0.9f, neonGreen });
        platforms.push_back({ { 4260, defaultFloorY - 120, 160, 20 }, 0.0f, 0.0f, false, neonCyan, 0.0f });
    }

    // --- Gravity Flip segment: flip gravity, run on ceiling over a fixed distance ---
    {
        gravityPads.push_back({ { 4520.0f, defaultFloorY - 24, 56, 16 }, neonPurple, true });

        float ceilingStart = 4660.0f;

        for (int i = 1; i < 6; i++) {
            float x = ceilingStart + i * 300.0f - i * i / 2 * 8;
            AddSpikeCluster(spikes, x, 4, 35.0f, 50.0f, false,
                neonMagenta, defaultFloorY, ceilingYTop);
        }

        AddSpikeCluster(spikes, ceilingStart - 40.0f, 30, 36.0f, 70.0f, true,
            neonYellow, defaultFloorY, ceilingYTop);

        gravityPads.push_back({ { ceilingStart + 8.5f * 200.0f, ceilingYTop + 6.0f, 56, 16 }, neonPurple, false });

        AddSpikeCluster(spikes, ceilingStart + 10.0f * 200.0f, 8, 36.0f, 70.0f, false,
            neonYellow, defaultFloorY, ceilingYTop);
    }

    // --- Jumpad trick ---
    {
        float trickStart = 6800.0f;
        platforms.push_back({ { trickStart + 475.0f,  defaultFloorY - 84, 140, 20 }, 0, 0.0f, false, neonCyan, 0.0f });
        jumpPads.push_back({ { trickStart + 400.0f, defaultFloorY - 32, 60, 16 }, 1.45f, neonYellow });
        AddSpikeCluster(spikes, trickStart + 675.0f, 4, 36.0f, 70.0f, true,
            neonMagenta, defaultFloorY, ceilingYTop);
    }

    // --- long Jump ---
    {
        float longJump = 7700.0f;
        speedPads.push_back({ { longJump + 475.0f, defaultFloorY - 8, 66, 8 }, 1.35f, 2.0f, neonGreen });
        AddSpikeCluster(spikes, longJump + 675.0f, 6, 34.0f, 40.0f, true,
            neonMagenta, defaultFloorY, ceilingYTop);
    }

    // --- Semi-difficult segment ---
    float extStart = 9100.0f;

    {
        speedPads.push_back({
            { extStart + 10.0f, defaultFloorY - 8, 66, 8 },
            1.5f,
            4.3f,
            neonGreen
            });

        for (int i = 0; i < 10; i++) {
            float px = extStart + 250.0f + i * 240.0f;
            float py = defaultFloorY - (80 + 30 * (i % 3));

            if (i % 2 == 0) {
                ghostPlatforms.push_back({
                    { px, py - 15, 150, 20 },
                    0.0f, 0.0f, false, neonBlue, 0.0f
                    });
            }
            else if (i == 1) {
                platforms.push_back({
                    { px, py + 20, 150, 20 },
                    0.0f, 0.0f, false, neonPurple, 0.0f
                    });
            }
            else {
                platforms.push_back({
                    { px, py, 150, 20 },
                    0.0f, 0.0f, false, neonPurple, 0.0f
                    });
            }

            AddSpikeCluster(spikes, px + 60.0f, 3, 34.0f, 60.0f, true,
                neonMagenta, defaultFloorY, ceilingYTop);
        }
    }

    // --- Gravity Pad sequence ---
    float gstart = extStart + 3000.0f;
    {
        for (int i = 0; i < 4; i++) {
            bool flipUp = (i % 2 == 0);

            gravityPads.push_back({
                { gstart + i * 300.0f, flipUp ? defaultFloorY - 24 : ceilingYTop + 6, 56, 16 },
                neonPurple,
                flipUp
                });

            AddSpikeCluster(spikes,
                gstart + i * 300.0f + 160.0f,
                3,
                36.0f,
                70.0f,
                flipUp ? false : true,
                neonMagenta,
                defaultFloorY,
                ceilingYTop);
        }
    }

    // --- Slowpad + spike run ---
    float tstart = gstart + 1900.0f;
    {
        float sx = tstart + 160.0f;
        speedPads.push_back({ { tstart, defaultFloorY - 8, 66, 8 }, 0.7f, 8.0f, neonBlue });
        for (int i = 0; i < 16; i++) {
            sx += 150.0f - 6.6f * i / 2;
            AddSpikeCluster(spikes, sx, 2, 34.0f, 40.0f, (i % 2 == 0),
                neonYellow, defaultFloorY, ceilingYTop);
        }
    }

    // --- Final chaos segment (red section) ---
    float chaosStart = tstart + 5700.0f;

    {
        // long speed pad
        speedPads.push_back({
            { chaosStart - 1720.0f, defaultFloorY - 8.0f, 300.0f, 8.0f },
            2.0f,
            12.0f,
            neonRed
            });

        AddSpikeCluster(spikes, chaosStart - 40.0f, 5, 34.0f, 45.0f, false,
            neonMagenta, defaultFloorY, ceilingYTop);

        float ax = chaosStart + 260.0f;
        for (int i = 0; i < 8; ++i) {
            float baseX = ax + i * 280.0f;

            float platY = defaultFloorY - (110.0f + 35.0f * ((i * 2) % 3));
            Color platColor =
                (i % 3 == 0) ? neonCyan :
                ((i % 3 == 1) ? neonPurple : neonBlue);

            if (i % 2 == 0) {
                platforms.push_back({
                    { baseX, platY, 150.0f, 20.0f },
                    0.0f, 0.0f, false,
                    platColor,
                    0.0f
                    });
            }

            bool spikesUp = (i == 0 || i == 2 || i == 5 || i == 7);
            AddSpikeCluster(spikes,
                baseX - (spikesUp ? 60.0f : 10.0f),
                3 + (i % 2),
                34.0f,
                60.0f,
                true,
                neonMagenta,
                defaultFloorY,
                ceilingYTop);

            if (i % 3 == 1) {
                AddSpikeCluster(spikes,
                    baseX + 40.0f,
                    2,
                    30.0f,
                    45.0f,
                    false,
                    neonMagenta,
                    defaultFloorY,
                    ceilingYTop);
            }
        }
    }

    // --- Multi-phase gravity tunnel ---
    float gravStart = chaosStart + 2700.0f;

    {
        gravityPads.push_back({
            { gravStart, defaultFloorY - 24.0f, 56.0f, 16.0f },
            neonPurple,
            true
            });

        float ceilingRunY = ceilingYTop + 70.0f;
        for (int i = 0; i < 3; ++i) {
            float px = gravStart + 350.0f + i * 260.0f;

            platforms.push_back({
                { px, ceilingRunY, 170.0f, 20.0f },
                0.0f, 0.0f, false,
                neonCyan,
                0.0f
                });

            AddSpikeCluster(spikes, px - 30.0f, 2, 34.0f, 50.0f, false,
                neonMagenta, defaultFloorY, ceilingYTop);

            AddSpikeCluster(spikes, px - 30.0f, 3, 34.0f, 70.0f, true,
                neonMagenta, defaultFloorY, ceilingYTop);
        }

        float gravMid1 = gravStart + 480.0f + 3.0f * 260.0f + 60.0f;
        gravityPads.push_back({
            { gravMid1, ceilingYTop + 6.0f, 56.0f, 16.0f },
            neonPurple,
            false
            });

        for (int i = 0; i < 3; ++i) {
            float gx = gravMid1 + 330.0f + i * 220.0f;
            float gy = defaultFloorY - (90.0f + 25.0f * (i % 2));

            platforms.push_back({
                { gx, gy, 140.0f, 20.0f },
                0.0f, 0.0f, false,
                neonRed,
                0.0f
                });

            AddSpikeCluster(spikes, gx - 40.0f, 2, 34.0f, 60.0f, true,
                neonMagenta, defaultFloorY, ceilingYTop);

            AddSpikeCluster(spikes, gx + 40.0f, 2, 30.0f, 45.0f, false,
                neonMagenta, defaultFloorY, ceilingYTop);
        }

        float gravExitX = gravMid1 + 460.0f + 2.0f * 260.0f + 160.0f;
        float finalStart = gravExitX + 260.0f;

        AddSpikeCluster(spikes, finalStart - 40.0f, 2, 30.0f, 50.0f, false,
            neonMagenta, defaultFloorY, ceilingYTop);

        jumpPads.push_back({
            { finalStart, defaultFloorY - 32.0f, 60.0f, 16.0f },
            1.75f,
            neonYellow
            });

        platforms.push_back({
            { finalStart + 260.0f, defaultFloorY - 190.0f, 220.0f, 22.0f },
            0.0f,
            0.0f,
            false,
            neonPurple,
            0.0f
            });

        AddSpikeCluster(spikes, finalStart + 220.0f, 3, 34.0f, 50.0f, false,
            neonMagenta, defaultFloorY, ceilingYTop);

        AddSpikeCluster(spikes, finalStart + 260.0f - 40.0f, 5, 34.0f, 70.0f, true,
            neonMagenta, defaultFloorY, ceilingYTop);

        // final finish line
        finishLine = { finalStart + 2500.0f, 0.0f, 8.0f, 720.0f };
    }
}
