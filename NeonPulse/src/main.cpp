#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <fstream>

#include "utils/utils.h"
#include "entities/entities.h"
#include "background/background.h"
#include "level/level.h" 
#include "story/story.h" 

using namespace std;


// ----------------------------------------
// Story bloclk text wrapping and drawing
// ----------------------------------------

void DrawWrappedTextBlockLeft(Font font, const  string& text, Rectangle bounds,
    float fontSize, float spacing, Color tint)
{
    float maxWidth = bounds.width;
    float lineSpacing = fontSize * 1.4f; // Distance between lines

    vector<string> lines;
    string currentLine;
    string currentWord;
    bool lastWasNewline = false;

    // Iterate one char past the end to flush last word
    for (size_t i = 0; i <= text.size(); ++i)
    {
        char c = (i < text.size()) ? text[i] : ' ';

        bool isSpace = (c == ' ' || c == '\t');
        bool isNewline = (c == '\n');
        bool isEnd = (i == text.size());

        if (isSpace || isNewline || isEnd)
        {
            // Flush current word into line
            if (!currentWord.empty())
            {
                 string candidate = currentLine.empty() ? currentWord : currentLine + " " + currentWord;

                float width = MeasureTextEx(font, candidate.c_str(), fontSize, spacing).x;

                if (width <= maxWidth || currentLine.empty())
                {
                    // Word fits into current line
                    currentLine = candidate;
                }
                else
                {
                    // Word does not fit, push current line and start new one
                    if (!currentLine.empty())
                        lines.push_back(currentLine);

                    currentLine = currentWord;
                }

                currentWord.clear();
                lastWasNewline = false;
            }

            if (isNewline)
            {
                // Manual line break: push current line, or empty line for double newline
                if (!currentLine.empty())
                {
                    lines.push_back(currentLine);
                    currentLine.clear();
                }
                else if (lastWasNewline)
                {
                    // Two newlines -> explicit empty line
                    lines.push_back("");
                }

                lastWasNewline = true;
            }
        }
        else
        {
            // Normal character, build word
            currentWord.push_back(c);
            lastWasNewline = false;
        }
    }

    // Flush remaining line
    if (!currentLine.empty())
    {
        lines.push_back(currentLine);
    }

    // Drawing phase
    Vector2 pos = { bounds.x, bounds.y };
    float bottom = bounds.y + bounds.height;

    for (const  string& line : lines)
    {
        if (pos.y + fontSize > bottom)
            break; // No more vertical space

        DrawTextEx(font, line.c_str(), pos, fontSize, spacing, tint);
        pos.y += lineSpacing;
    }
}


int main() {
    const int screenW = 1280;
    const int screenH = 720;
    InitWindow(screenW, screenH, "Neon Pulse");

    // --------------------
	// Audio setup
    // --------------------

    InitAudioDevice();

    // Load menu ambient music 
    Music menuMusic = LoadMusicStream("audio/menu_ambient.mp3");
    menuMusic.looping = true;
    SetMusicVolume(menuMusic, 0.2f); 

    // Load main level music 
    Music levelMusic = LoadMusicStream("audio/level_theme.mp3");
    levelMusic.looping = true;
    SetMusicVolume(levelMusic, 0.2f); 

    // At game start we are in menu, so start menu music immediately
    PlayMusicStream(menuMusic);
    bool menuMusicPlaying = true;
    bool levelMusicPlaying = false;

    auto SwitchToMenuMusic = [&]() {
        // Stop level music if it is playing and switch to ambient
        if (levelMusicPlaying) {
            StopMusicStream(levelMusic);
            levelMusicPlaying = false;
        }
        if (!menuMusicPlaying) {
            // Start or resume ambient menu music
            PlayMusicStream(menuMusic);
            menuMusicPlaying = true;
        }
        };

    auto SwitchToLevelMusicRestart = [&]() {
        // Switch from ambient to level music and always restart track from the beginning
        if (menuMusicPlaying) {
            StopMusicStream(menuMusic);
            menuMusicPlaying = false;
        }
        StopMusicStream(levelMusic);   // reset
        PlayMusicStream(levelMusic);   // start playing from the beginning
        levelMusicPlaying = true;
        };

    // --------------------------------
	// Story functions initialization
    // --------------------------------

    StoryProgress storyProgress;
    LoadStoryProgress(storyProgress);   // Load player info from progress file
    StoryState storyState;              // Runtime state for current story overlay

    // ------------------------------------------
	// Variable initialization (diferent types)
    // ------------------------------------------

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
    bool grounded = false;
    bool alive = true;
    float deathShake = 0.0f;
    bool holdJumpActive = false;
    bool prevGrounded = false;
    const float gravityBase = 2300.0f;
    const float jumpVelBase = -760.0f; // base jump velocity
    int gravityDir = 1; // 1 = normal (gravity pulls down), -1 = inverted (gravity pulls up)
    float gravityFlipTimer = 0.0f; // gravity flip cooldown
    
    // Level
	const float gravityFlipCooldown = 0.35f; // seconds between gravity flips
    bool levelFinished = false;
	float endScreenTimer = 0.0f;          // how long we have been in end screen state
    const float endScreenDelay = 1.5f;    // delay before showing overlay
	bool overlayMusicStarted = false;     // whether end screen music has started

    //Story
    const int TOTAL_STORY_CHAPTERS = 11;

    // SpeedPad state
    float speedTimer = 0.0f;
    float speedMultiplierActive = 1.0f;

    // Start / respawn menus
    bool inStartMenu = true;     // start in main menu
    float respawnHold = 0.0f;    // hold progress for respawn
    const float respawnFillSpeed = 0.4f;   // how fast bar fills
    const float respawnDecaySpeed = 2.4f;  // how fast bar empties when released

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
    Color neonRed = { 255, 49, 49, 255 };

    // Sections (only visual)
    vector<Section> sections = {
        { 0.0f,     1200.0f,  { 20, 30, 60, 255 }, { 40, 10, 80, 255 } },
        { 1200.0f,  2600.0f,  { 10, 50, 80, 255 }, { 0, 20, 40, 255 } },
        { 2600.0f,  4200.0f,  { 10, 10, 40, 255 }, { 40, 0, 60, 255 } },
        { 4200.0f,  18350.0f,  { 8, 12, 26, 255  }, { 18, 26, 64, 255 } },
        { 18350.0f, 29000.0f,  { 80, 0, 0, 255 }, { 180, 0, 0, 255 } }
    };

    // Parallax layers
    vector<ParallaxLayer> layers = {
        { 0.06f, neonBlue,   16, 10.0f, 30.0f },
        { 0.12f, neonPurple, 20, 6.0f,  20.0f },
        { 0.22f, neonCyan,   28, 4.0f,  14.0f },
    };

	// Vectors to hold level entities
    vector<MovingPlatform> platforms;
    vector<Spike> spikes;
    vector<Arch> arches;
    vector<JumpPad> jumpPads;
    vector<SpeedPad> speedPads;
    vector<GhostPlatform> ghostPlatforms;
    vector<GravityPad> gravityPads;

    // helper to add spike clusters
    auto addSpikeClusterLocal = [&](float startX, int count, float w, float h, bool up, Color c) {
        for (int i = 0; i < count; i++) {
            float x = startX + i * (w * 0.86f);
            float y = up ? (defaultFloorY - h) : (ceilingYTop);
            spikes.push_back({ { x, y, w, h }, up, c });
        }
     };

    Rectangle finishLine{};


	// Build the level
    BuildLevel(
        platforms,
        ghostPlatforms,
        spikes,
        jumpPads,
        speedPads,
        gravityPads,
        finishLine,
        defaultFloorY,
        ceilingYTop,
        neonCyan,
        neonMagenta,
        neonYellow,
        neonGreen,
        neonBlue,
        neonPurple,
        neonRed
    );


        // Particle container
        vector<Particle> particles;
        particles.reserve(400);

        // reset full game state for a new run
        auto ResetRun = [&]() {
            player = { 100, 520, 36, 36 };
            playerVel = { 0.0f, 0.0f };
            grounded = false;
            alive = true;

            // reset camera and timing
            camX = 0.0f;
            songTime = 0.0f;
            deathShake = 0.0f;

            // clear particles
            particles.clear();

            // reset speed to default
            baseRunSpeed = baseRunSpeedDefault;
            runSpeed = baseRunSpeedDefault;
            speedTimer = 0.0f;
            speedMultiplierActive = 1.0f;

            // auto-jump flags
            holdJumpActive = false;
            prevGrounded = false;

            // gravity state
            gravityDir = 1;
            gravityFlipTimer = 0.0f;

            // level state
            levelFinished = false;

            // respawn bar
            respawnHold = 0.0f;

            // end screen delay timer
            endScreenTimer = 0.0f;

            // Allow overlay logic and music to start fresh on next ending
            overlayMusicStarted = false;         

            // Clear runtime story state
            storyState.active = false;           
            storyState.blocks.clear();           
            storyState.currentBlock = 0;          
            storyState.chapterIndex = 0;          
            storyState.fullyRead = false;               
            };


        // Beat pulse function
        auto BeatPulse = [&](float t) {
        float beat = fmodf(t, secondsPerBeat);
        return expf(-6.0f * beat);
        };

    // current section
    auto CurrentSection = [&](float x) -> Section {
        for (auto& s : sections) {
            if (x >= s.startX && x < s.endX) return s;
        }
        return sections.back();
        };



    // ----------------
	// Main game loop
    // ----------------

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        songTime += dt;
        float pulse = BeatPulse(songTime); // global pulse for this frame

        // Update only currently playing music streams
        if (menuMusicPlaying) {
            UpdateMusicStream(menuMusic); // Keep streaming menu ambient
        }
        if (levelMusicPlaying) {
            UpdateMusicStream(levelMusic); // Keep streaming level music
        }

        if (inStartMenu) {
            // --- Start game input ---

            // button geometry
            int btnW = 260;
            int btnH = 80;
            int btnX = screenW / 2 - btnW / 2;
            int btnY = screenH / 2;

            Rectangle playButton = { (float)btnX, (float)btnY, (float)btnW, (float)btnH };

            Vector2 mouse = GetMousePosition();
            bool hovered = CheckCollisionPointRec(mouse, playButton);
            bool clicked = hovered && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);

            // start only with ENTER or clicking PLAY
            if (IsKeyPressed(KEY_ENTER) || clicked) {
                ResetRun();        // reset all game state
                inStartMenu = false;

				// music switch
                SwitchToLevelMusicRestart();
            }

            BeginDrawing();
            ClearBackground(BLACK);

            // animated background even in menu
            Section sec = CurrentSection(camX + screenW * 0.5f);
            DrawBackground(screenW, screenH, sec, camX, layers, pulse);

            // title
            const char* title = "NEON PULSE";
            int titleSize = 64;
            int tw = MeasureText(title, titleSize);
            DrawText(title, screenW / 2 - tw / 2, screenH / 4, titleSize, Fade(WHITE, 0.95f));

            // big PLAY button
            Color btnFill = hovered ? Fade(neonGreen, 0.95f) : Fade(neonCyan, 0.85f);
            Color btnBorder = Fade(WHITE, 0.9f);
            DrawRectangleRounded(playButton, 0.25f, 10, btnFill);
            DrawRectangleLinesEx(playButton, 3.0f, btnBorder);

            const char* playText = "PLAY";
            int playSize = 32;
            int pw = MeasureText(playText, playSize);
            DrawText(playText, screenW / 2 - pw / 2, btnY + btnH / 2 - playSize / 2, playSize, Fade(BLACK, 0.9f));

            // hint text
            const char* pressMsg = "Press ENTER or click PLAY";
            int msgSize = 20;
            int mw = MeasureText(pressMsg, msgSize);
            DrawText(pressMsg, screenW / 2 - mw / 2, btnY + btnH + 40, msgSize, Fade(WHITE, 0.85f));

            EndDrawing();
            continue;   // skip gameplay while in start menu
        }



        // --- Jump ---
        if (alive) {
            // Update the holding state
            holdJumpActive = (IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_UP));

            // Immediate single jump on key press
            if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP)) && grounded) {
                playerVel.y = jumpVelBase * (float)gravityDir;
                grounded = false;

                // jump particles
                for (int i = 0; i < 10; ++i) {
                    // corrected GetRandomValue range ordering
                    float ang = (float)GetRandomValue(-100, -80) * DEG2RAD;
                    float sp = (float)GetRandomValue(160, 320);
                    particles.push_back({
                        { player.x + player.width * 0.5f, player.y + player.height },
                        { cosf(ang) * sp, sinf(ang) * sp * -1.0f },
                        0.45f + GetRandomValue(0,20) * 0.01f,
                        (float)GetRandomValue(2,6),
                        Fade(neonYellow, 0.9f)
                        });
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

        // gravity flip cooldown decrement
        if (gravityFlipTimer > 0.0f) gravityFlipTimer = max(0.0f, gravityFlipTimer - dt);


        // player horizontal control
        if (alive) playerVel.x = runSpeed;
        else playerVel.x = 0.0f;
        if (levelFinished) playerVel.x = 0.0f;


        // gravity
        playerVel.y += gravityBase * (float)gravityDir * dt;


        // integrate
        if (alive) {
            player.x += playerVel.x * dt;
            player.y += playerVel.y * dt;
        }

        // Floor / ceiling collision handling with gravity direction awareness
        grounded = false;
        if (gravityDir > 0) {
			// normal gravity
            if (player.y + player.height >= defaultFloorY) {
                player.y = defaultFloorY - player.height;
                playerVel.y = 0.0f;
                grounded = true;
            }
            if (player.y <= ceilingYTop) {
                player.y = ceilingYTop;
                if (playerVel.y < 0.0f) playerVel.y = 0.0f;
            }
        }
        else {
            // inverted gravity
            if (player.y <= ceilingYTop) {
                player.y = ceilingYTop;
                playerVel.y = 0.0f;
                grounded = true;
            }
            if (player.y + player.height >= defaultFloorY) {
                player.y = defaultFloorY - player.height;
                if (playerVel.y > 0.0f) playerVel.y = 0.0f;
            }
        }

        // Static platforms collision + resolve
        float tPhase = songTime + pulse * 0.03f; // still used later for ghost platforms, keep it

        for (auto& p : platforms) {
            // For static platforms we can use the base rectangle directly
            Rectangle pr = p.base;

            // Simple horizontal culling by camera, to skip far platforms
            if (pr.x + pr.width < player.x - 300.0f || pr.x > player.x + 900.0f) continue;

            if (!RectsIntersect(player, pr)) continue;

            // Previous frame player rect (used to detect collision side)
            Rectangle prevPlayer = {
                player.x - playerVel.x * dt,
                player.y - playerVel.y * dt,
                player.width,
                player.height
            };

            // Determine contact sides based on previous position
            bool fromTop = (prevPlayer.y + prevPlayer.height <= pr.y + 1.0f);
            bool fromBottom = (prevPlayer.y >= pr.y + pr.height - 1.0f);
            bool fromLeft = (prevPlayer.x + prevPlayer.width <= pr.x + 1.0f);
            bool fromRight = (prevPlayer.x >= pr.x + pr.width - 1.0f);

            if (gravityDir > 0) {
                // Normal gravity
                if (fromTop) {
                    player.y = pr.y - player.height;
                    playerVel.y = 0.0f;
                    grounded = true;
                }
                else if (fromBottom) {
                    // Hit platform from below
                    player.y = pr.y + pr.height;
                    if (playerVel.y < 0.0f) playerVel.y = 0.0f;
                }
                else if (fromLeft) {
                    player.x = pr.x - player.width;
                }
                else if (fromRight) {
                    player.x = pr.x + pr.width;
                }
            }
            else {
                // Inverted gravity
                if (fromBottom) {
                    player.y = pr.y + pr.height;
                    playerVel.y = 0.0f;
                    grounded = true;
                }
                else if (fromTop) {
                    player.y = pr.y - player.height;
                    if (playerVel.y > 0.0f) playerVel.y = 0.0f;
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
                playerVel.y = jumpVelBase * gravityDir * jp.strength;
                grounded = false;
                // Jump pad particles
                for (int i = 0; i < 16; ++i) {
                    float ang = (float)GetRandomValue(-110, -70) * DEG2RAD;
                    float sp = (float)GetRandomValue(220, 420);
                    particles.push_back({ { player.x + player.width * 0.5f, player.y + player.height }, { cosf(ang) * sp, sinf(ang) * sp * -1.0f }, 0.5f + GetRandomValue(0,20) * 0.01f, (float)GetRandomValue(3,7), Fade(jp.color, 0.95f) });
                }
            }
        }

        // SpeedPad activation
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

        // GravityPad activation
        for (const auto& gp : gravityPads) {
            if (RectsIntersect(player, gp.rect) && gravityFlipTimer <= 0.0f) {
                // flip gravity
                gravityDir = -gravityDir;
                gravityFlipTimer = gravityFlipCooldown;

                // reset vertical velocity for predictability
                playerVel.y = 0.0f;

                if (gravityDir < 0) {
                    // place player just below the ceiling so they land/stand on it
                    player.y = ceilingYTop + 0.5f; // small offset to avoid overlapping spike geometry
                    grounded = true;
                    prevGrounded = true;
                }
                else {
                    // place player on floor
                    player.y = defaultFloorY - player.height - 0.5f;
                    grounded = true;
                    prevGrounded = true;
                }

                // visual particle burst to indicate flip
                for (int i = 0; i < 20; ++i) {
                    float ang = (float)GetRandomValue(0, 360) * DEG2RAD;
                    float sp = (float)GetRandomValue(120, 420);
                    particles.push_back({ { player.x + player.width * 0.5f, player.y + player.height * 0.5f }, { cosf(ang) * sp, sinf(ang) * sp }, 0.5f + GetRandomValue(0,20) * 0.01f, (float)GetRandomValue(2,6), Fade(gp.color, 0.9f) });
                }
            }

        }

        // Finish line detection
        if (alive && !levelFinished && RectsIntersect(player, finishLine)) {
            levelFinished = true;

            // Stop player movement
            playerVel = { 0, 0 };


            // Victory particles
            for (int i = 0; i < 60; ++i) {
                float ang = (float)GetRandomValue(0, 360) * DEG2RAD;
                float sp = (float)GetRandomValue(120, 480);
                particles.push_back({
                    { player.x + player.width * 0.5f, player.y + player.height * 0.5f },
                    { cosf(ang) * sp, sinf(ang) * sp },
                    0.8f + GetRandomValue(0, 30) * 0.01f,
                    (float)GetRandomValue(3, 7),
                    Fade(neonGreen, 0.9f)
                    });
            }
        }



        // Spike collision = death
        for (const auto& s : spikes) {
            if (player.x + player.width > s.base.x - 20 && player.x < s.base.x + s.base.width + 20) {
                if (CollideSpike(player, s)) {
                    alive = false;
                    deathShake = 8.0f;

                    // Music will change when respawn menu overlay appears.
                    break;
                }
            }
        }


		// Auto-jump on landing
        if (alive) {
            // Landing detection
            if (!prevGrounded && grounded && holdJumpActive) {
                // immediate auto-jump
                playerVel.y = jumpVelBase * (float)gravityDir;
                grounded = false;

                // jump particles
                for (int i = 0; i < 10; ++i) {
                    float ang = (float)GetRandomValue(-100, -80) * DEG2RAD;
                    float sp = (float)GetRandomValue(160, 320);
                    particles.push_back({
                        { player.x + player.width * 0.5f, player.y + player.height },
                        { cosf(ang) * sp, sinf(ang) * sp * -1.0f },
                        0.45f + GetRandomValue(0,20) * 0.01f,
                        (float)GetRandomValue(2,6),
                        Fade(neonYellow, 0.9f)
                        });
                }
            }
        }
        // Respawn / restart
        bool runOver = (!alive || levelFinished);

        if (runOver) {
            // Increase end-screen timer while we stay in this state
            endScreenTimer += dt;

            if (endScreenTimer >= endScreenDelay) {
                bool holdingR = IsKeyDown(KEY_R);

                if (holdingR) {
                    // Fill progress bar while R is held
                    respawnHold += respawnFillSpeed * dt;
                }
                else {
                    // Decay progress when R is released
                    respawnHold -= respawnDecaySpeed * dt;
                }

                // Clamp progress to [0, 1]
                if (respawnHold < 0.0f) respawnHold = 0.0f;
                if (respawnHold > 1.0f) respawnHold = 1.0f;

                // When bar is full -> restart the run
                if (respawnHold >= 1.0f) {
                    ResetRun();
                    inStartMenu = false;  // go straight back into gameplay
                    respawnHold = 0.0f;

                    SwitchToLevelMusicRestart();
                }
            }
            else {
                // During delay we do not want any leftover fill
                respawnHold -= respawnDecaySpeed * dt;
                if (respawnHold < 0.0f) respawnHold = 0.0f;
            }
        }
        else {
            // While run is active, timer stays at zero and bar drains
            endScreenTimer = 0.0f;
            respawnHold -= respawnDecaySpeed * dt;
            if (respawnHold < 0.0f) respawnHold = 0.0f;
        }




        // Camera follows player
        camX = player.x - 280.0f;


        // Particles update and cleanup
        for (int i = (int)particles.size() - 1; i >= 0; --i) {
            particles[i].life -= dt;
            if (particles[i].life <= 0.0f) {
                particles[i] = particles.back();
                particles.pop_back();
                continue;
            }
            particles[i].pos.x += particles[i].vel.x * dt;
            particles[i].pos.y += particles[i].vel.y * dt;
            particles[i].vel.x *= (1.0f - 3.0f * dt);
            particles[i].vel.y += 500.0f * dt;
        }

        if (deathShake > 0.0f) deathShake = max(0.0f, deathShake - 24.0f * dt);

        // ----------------
		// Render
        // ----------------
        BeginDrawing();
        ClearBackground(BLACK);

        float shakeX = (GetRandomValue(-1000, 1000) / 1000.0f) * deathShake;
        float shakeY = (GetRandomValue(-1000, 1000) / 1000.0f) * deathShake;

        Section sec = CurrentSection(camX + screenW * 0.5f);
        DrawBackground(screenW, screenH, sec, camX, layers, pulse);


        // Floor and ceiling rails
        Color railA = Fade(neonBlue, 0.45f + 0.2f * pulse);
        Color railB = Fade(neonPurple, 0.45f + 0.2f * pulse);
        DrawRectangleGradientH(0, (int)defaultFloorY, screenW, 6, railA, railB);
        DrawRectangleGradientH(0, (int)ceilingYTop - 6, screenW, 6, railB, railA);

		// Draw speedpads, jumppads, gravitypads
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
        for (const auto& gp : gravityPads) {
            float x = gp.rect.x - camX;
            if (x + gp.rect.width < -120 || x > screenW + 120) continue;

            DrawRectangleRounded({ x, gp.rect.y, gp.rect.width, gp.rect.height }, 0.25f, 6, Fade(gp.color, 0.92f));
            DrawRectangleLinesEx({ x, gp.rect.y, gp.rect.width, gp.rect.height }, 2.0f, Fade(WHITE, 0.08f));

            // small icon to suggest flip
            Vector2 center = { x + gp.rect.width * 0.5f, gp.rect.y + gp.rect.height * 0.5f };
            Vector2 t1, t2, t3;
            if (gp.flipsUp) {
                // Up arrow
                t1 = { center.x, center.y - 6.0f };            
                t2 = { center.x - 6.0f, center.y + 6.0f };    
                t3 = { center.x + 6.0f, center.y + 6.0f };     

                // draw filled triangle
                DrawTriangle(t1, t2, t3, Fade(WHITE, 0.85f));
                DrawTriangleLines(t1, t2, t3, Fade(BLACK, 0.25f));
            }
            else {
                // Down arrow
                t1 = { center.x, center.y + 6.0f };            
                t2 = { center.x + 6.0f, center.y - 6.0f };     
                t3 = { center.x - 6.0f, center.y - 6.0f };     

                // draw filled triangle
                DrawTriangle(t1, t2, t3, Fade(WHITE, 0.85f));
                DrawTriangleLines(t1, t2, t3, Fade(BLACK, 0.25f));
            }
        }


        // Patforms
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

        // Ghost platforms
        for (const auto& gp : ghostPlatforms) {
            Rectangle r = gp.GetRect(tPhase);
            if (r.x + r.width - camX < -160 || r.x - camX > screenW + 160) continue;
            Rectangle drawR = { r.x - camX + shakeX, r.y + shakeY, r.width, r.height };
            Color fill = Fade(gp.color, 0.45f + 0.28f * pulse);
            Color edge = Fade(gp.color, 0.96f);
            DrawRectangleRounded(drawR, 0.18f, 6, fill);
            DrawRectangleLinesEx(drawR, 3.0f, edge);
            DrawRectangle((int)drawR.x, (int)(drawR.y + drawR.height), (int)drawR.width, 6, Fade(gp.color, 0.28f));
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
            DrawCircleV(ppos, prt.size, Fade(prt.color, Clamp1(prt.life * 2.5f, 0.0f, 1.0f)));
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
        DrawCircleLines((int)(drawPlayer.x + drawPlayer.width * 0.5f), (int)(drawPlayer.y + drawPlayer.height * 0.5f), ringR, Fade(neonYellow, 0.6f * pulse));

        // Finish line visual
        if (finishLine.x - camX < screenW + 200) {
            DrawRectangle((int)(finishLine.x - camX), 0, 4, screenH, Fade(neonGreen, 0.95f));
            DrawText("FINISH", (int)(finishLine.x - camX) + 30, screenH / 2 - 12, 20, Fade(WHITE, 0.9f));
        }

        // HUD
        DrawText("Neon Pulse", 24, 20, 28, Fade(WHITE, 0.9f));
        DrawText(TextFormat("Jump: Space/Up | Retry: hold R"), 24, 56, 20, Fade(WHITE, 0.6f));

		// Speed display
        if (speedTimer > 0.0f) {
            DrawText(TextFormat("SPEED x%.2f (%.1fs)", speedMultiplierActive, speedTimer), 24, 108, 18, Fade(neonGreen, 0.9f));
        }


		// overlay menu with story in it
        if ((!alive || levelFinished) && endScreenTimer >= endScreenDelay) {

            if (!overlayMusicStarted) {
                // Switch music
                SwitchToMenuMusic();
                overlayMusicStarted = true;

                // Start story chapter for this ending (death or victory)
                StartStoryForCurrentEnding(storyProgress, storyState);
            }

            // --- Story input update ---
            if (storyState.active) {

                // Left mouse click shows next block
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    if (storyState.currentBlock < (int)storyState.blocks.size()) {
                        storyState.currentBlock++;

                        // If we just reached the last block -> mark chapter as fully read
                        if (storyState.currentBlock >= (int)storyState.blocks.size()) {
                            storyState.fullyRead = true;
                            MarkChapterCompletedIfNeeded(storyProgress, storyState);
                        }
                    }
                }
            }

            // Darken the whole screen
            DrawRectangle(0, 0, screenW, screenH, Fade(BLACK, 0.65f));

            // Large story panel (almost full screen height)
            int panelW = (int)(screenW * 0.8f);
            int panelH = (int)(screenH * 0.85f);
            int panelX = screenW / 2 - panelW / 2;
            int panelY = screenH / 2 - panelH / 2;

            Rectangle panelRect = { (float)panelX, (float)panelY, (float)panelW, (float)panelH };

            if (storyState.active && storyState.chapterIndex > 0) {
                int progFontSize = 20;
                const char* progText = TextFormat("%d / %d", storyState.chapterIndex, TOTAL_STORY_CHAPTERS);

				// progress text position
                int progWidth = MeasureText(progText, progFontSize);
                int progX = (int)(panelRect.x + panelRect.width - progWidth - 28); 
                int progY = (int)(panelRect.y + 30);                               

                DrawText(progText, progX, progY, progFontSize, WHITE);
            }

            // Panel background color
            Color panelFill = { 8, 10, 24, 235 }; // Dark blue-ish background
            DrawRectangleRounded(panelRect, 0.08f, 12, panelFill);
            DrawRectangleLinesEx(panelRect, 3.0f, Fade(neonCyan, 0.9f));

            // Choose texts based on game state
            const char* titleText;
            const char* holdText;
            Color titleColor;

            if (!alive && !levelFinished) {
                // Death case
                titleText = "YOU DIED";
                holdText = "Hold R to respawn, but don't forget about the story";
                titleColor = neonRed;
            }
            else {
                // Level finished case
                titleText = "LEVEL COMPLETE";
                holdText = "Hold R to restart, but don't forget about the story";
                titleColor = neonGreen;
            }

            // Title
            int titleSize = 32;
            int tw = MeasureText(titleText, titleSize);
            DrawText(titleText, screenW / 2 - tw / 2, panelY + 24, titleSize, titleColor);

            // Story text area inside panel
            Rectangle storyRect = {
                panelRect.x + 40.0f,
                panelRect.y + 80.0f,
                panelRect.width - 80.0f,
                panelRect.height - 230.0f
            };


            if (storyState.active && storyState.currentBlock > 0) {
                // We show only the current block, previous text is not visible anymore
                int index = storyState.currentBlock - 1;
                if (index < 0) index = 0;
                if (index >= (int)storyState.blocks.size()) {
                    index = (int)storyState.blocks.size() - 1;
                }

                const  string& blockText = storyState.blocks[index];

                DrawWrappedTextBlockLeft(GetFontDefault(), blockText, storyRect, 22.0f, 2.0f, Fade(WHITE, 0.96f));
            }
            else {
                // Fallback text if no story is available
                const char* fallbackMsg = (!alive && !levelFinished)
                    ? "You died. Try one more time, please;)"
                    : "Level complete.";

                DrawWrappedTextBlockLeft(GetFontDefault(), fallbackMsg, storyRect, 22.0f, 2.0f, Fade(WHITE, 0.96f));
            }


			// Click hint text
            const char* clickHint = nullptr;
            if (storyState.active && storyState.currentBlock < (int)storyState.blocks.size()) {
                clickHint = "Left mouse button: next line";
            }
            else if (storyState.active && !storyState.blocks.empty()) {
                clickHint = "Story finished: you can restart.";
            }

            if (clickHint) {
                int hintSize = 18;
                int hw = MeasureText(clickHint, hintSize);
                DrawText(clickHint, screenW / 2 - hw / 2, (int)(storyRect.y + storyRect.height - 18), hintSize, Fade(WHITE, 0.75f));
            }

            // Respawn / restart progress bar inside panel
            int barW = (int)(panelW * 0.6f);
            int barH = 26;
            int barX = screenW / 2 - barW / 2;
            int barY = panelY + panelH - barH - 70;

            Rectangle barBg = { (float)barX, (float)barY, (float)barW, (float)barH };
            DrawRectangleRounded(barBg, 0.3f, 8, Fade(BLACK, 0.7f));
            DrawRectangleLinesEx(barBg, 2.0f, Fade(WHITE, 0.8f));

            Rectangle barFill = {
                (float)barX,
                (float)barY,
                (float)(barW * respawnHold),
                (float)barH
            };
            DrawRectangleRounded(barFill, 0.3f, 8, Fade(neonGreen, 0.95f));

            // "Hold R" text above bar
            int htSize = 20;
            int htW = MeasureText(holdText, htSize);
            DrawText(holdText, screenW / 2 - htW / 2, barY - 32, htSize, Fade(WHITE, 0.9f));
        }

        EndDrawing();

        // update prevGrounded for the next frame's landing detection
        prevGrounded = grounded;

    }
    StopMusicStream(menuMusic);
    StopMusicStream(levelMusic);
    UnloadMusicStream(menuMusic);
    UnloadMusicStream(levelMusic);
    CloseAudioDevice();

    CloseWindow();
    return 0;
}



