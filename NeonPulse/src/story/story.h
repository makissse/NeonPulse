#pragma once

#include <vector>
#include <string>

// --------------------
// Story system
// --------------------

// Runtime state of currently shown story screen
struct StoryState {
    bool active = false;                    // Is story overlay currently visible
    std::vector<std::string> blocks;        // Text blocks split from markdown
    int currentBlock = 0;                   // Index of current block 
    int chapterIndex = 0;                   // Which chapter (death index) is used (1 - n)
    bool fullyRead = false;                 // True when last block has been shown
};

// Persistent progress across runs
struct StoryProgress {
    int endingsTotal = 0;                  
    int chapterCompleted = 0;              
};

// Load progress from disk, otherwise set defaults
void LoadStoryProgress(StoryProgress& progress);

// Decides which chapter to show and fills StoryState
void StartStoryForCurrentEnding(StoryProgress& progress, StoryState& story);

// It will update progress.chapterCompleted and save to disk
void MarkChapterCompletedIfNeeded(StoryProgress& progress, StoryState& story);
