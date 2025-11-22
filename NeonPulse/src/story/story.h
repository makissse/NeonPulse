// story/story.h
#pragma once

#include <vector>
#include <string>

// Runtime state of currently shown story screen
struct StoryState {
    bool active = false;                    // Is story overlay currently visible
    std::vector<std::string> blocks;        // Text blocks split from markdown
    int currentBlock = 0;                   // Index of current block (1..blocks.size())
    int chapterIndex = 0;                   // Which chapter (death index) is used (1..N)
    bool fullyRead = false;                 // True when last block has been shown
    float timeOnScreen = 0.0f;              // Time spent on current story screen (seconds)
};

// Persistent progress across runs
struct StoryProgress {
    int endingsTotal = 0;                   // Total number of endings (death or victory)
    int chapterCompleted = 0;               // How many chapters were fully read
};

// Load progress from disk (if file exists), otherwise set defaults
void LoadStoryProgress(StoryProgress& progress);

// Call this once for each ending (death or victory) when overlay appears
// Decides which chapter to show and fills StoryState
void StartStoryForCurrentEnding(StoryProgress& progress, StoryState& story);

// Call this when storyState.fullyRead becomes true
// It will update progress.chapterCompleted and save to disk
void MarkChapterCompletedIfNeeded(StoryProgress& progress, StoryState& story);
