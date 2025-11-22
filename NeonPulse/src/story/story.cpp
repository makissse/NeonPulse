// story/story.cpp
#include "story.h"

#include <fstream>   // For file IO
#include <string>    // For  string
#include <vector>    // For  vector

using namespace std;

// Max number of story files: death1.md ... death25.md
static const int MAX_STORY_FILES = 11;

// Relative path to progress file (relative to working directory)
static const char* STORY_PROGRESS_FILE = "story_text/story_progress.dat";

// -------------------------
// Internal helpers
// -------------------------

// Split markdown file into blocks separated by empty lines
static vector<string> LoadStoryBlocksFromMarkdown(const string& fileName)
{
    vector<string> blocks;

    // Open file using relative path
    ifstream in(fileName);
    if (!in.is_open()) {
        // File not found -> no story
        return blocks;
    }

    string line;
    string currentBlock;

    while (getline(in, line)) {
        if (line.empty()) {
            // Empty line -> new block
            if (!currentBlock.empty()) {
                blocks.push_back(currentBlock);
                currentBlock.clear();
            }
        }
        else {
            if (!currentBlock.empty()) {
                currentBlock += "\n"; // Keep line breaks inside block
            }
            currentBlock += line;
        }
    }

    // Push last block if not empty
    if (!currentBlock.empty()) {
        blocks.push_back(currentBlock);
    }

    return blocks;
}

// Save story progress into small text file: "endingsTotal chapterCompleted"
static void SaveStoryProgressInternal(const StoryProgress& p)
{
    ofstream out(STORY_PROGRESS_FILE);
    if (!out.is_open()) {
        // If we cannot open save file, just skip saving
        return;
    }

    // Simple format: "endingsTotal chapterCompleted"
    out << p.endingsTotal << " " << p.chapterCompleted;
}

// -------------------------
// Public API
// -------------------------

void LoadStoryProgress(StoryProgress& p)
{
    // Default values if file not found or broken
    p.endingsTotal = 0;
    p.chapterCompleted = 0;

    ifstream in(STORY_PROGRESS_FILE);
    if (!in.is_open()) {
        // No save file yet
        return;
    }

    int endings = 0;
    int completed = 0;

    // Try to read two integers from the file.
    // If this fails (non-numbers, empty file, etc.) – keep defaults (0, 0).
    if (!(in >> endings >> completed)) {
        return;
    }

    // Clamp to safe ranges
    if (endings < 0) endings = 0;
    if (completed < 0) completed = 0;
    if (completed > MAX_STORY_FILES) completed = MAX_STORY_FILES;

    p.endingsTotal = endings;
    p.chapterCompleted = completed;
}

void StartStoryForCurrentEnding(StoryProgress& progress, StoryState& story)
{
    // Count this ending (death or victory) and save immediately
    progress.endingsTotal += 1;
    SaveStoryProgressInternal(progress);

    // First ever ending: do NOT show any story
    if (progress.endingsTotal <= 1) {
        story.active = false;
        story.blocks.clear();
        story.currentBlock = 0;
        story.chapterIndex = 0;
        story.fullyRead = false;
        story.timeOnScreen = 0.0f;
        return;
    }

    // Chapter to show is always "completed + 1"
    // Example:
    //   chapterCompleted = 0 -> show chapter 1
    //   chapterCompleted = 1 -> show chapter 2, etc.
    int chapterToShow = progress.chapterCompleted + 1;

    // If we passed the last chapter -> no more story
    if (chapterToShow > MAX_STORY_FILES) {
        story.active = false;
        story.blocks.clear();
        story.currentBlock = 0;
        story.chapterIndex = 0;
        story.fullyRead = false;
        story.timeOnScreen = 0.0f;
        return;
    }

    // Build file name: "story_text/death1.md", "story_text/death2.md", ...
    string fileName = string("story_text/death") + to_string(chapterToShow) + ".md";

    vector<string> blocks = LoadStoryBlocksFromMarkdown(fileName);
    if (blocks.empty()) {
        // No content for this chapter -> no story
        story.active = false;
        story.blocks.clear();
        story.currentBlock = 0;
        story.chapterIndex = 0;
        story.fullyRead = false;
        story.timeOnScreen = 0.0f;
        return;
    }

    // Initialize runtime story state
    story.active = true;
    story.blocks =  move(blocks);
    story.currentBlock = 1;             // Start from first block
    story.chapterIndex = chapterToShow;
    story.fullyRead = false;
    story.timeOnScreen = 0.0f;
}

void MarkChapterCompletedIfNeeded(StoryProgress& progress, StoryState& story)
{
    // Only mark as completed if:
    // - story is active
    // - last block is reached
    // - chapterIndex is valid
    if (!story.active) return;
    if (!story.fullyRead) return;
    if (story.chapterIndex <= 0) return;

    // Only move forward if this chapter is new
    if (progress.chapterCompleted < story.chapterIndex) {
        progress.chapterCompleted = story.chapterIndex;
        SaveStoryProgressInternal(progress);
    }
}
