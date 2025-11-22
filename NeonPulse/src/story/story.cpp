#include "story.h"
#include <fstream>   
#include <string>    
#include <vector>    

using namespace std;

// --------------------
// Story variables
// --------------------

// Max number of story files
static const int MAX_STORY_FILES = 11;

// Relative path to progress file
static const char* STORY_PROGRESS_FILE = "story_text/story_progress.dat";

// -------------------------
// Internal helpers
// -------------------------

// Split markdown file into blocks separated by empty lines, no files -> no story
static vector<string> LoadStoryBlocksFromMarkdown(const string& fileName)
{
    vector<string> blocks;

    ifstream in(fileName);
    if (!in.is_open()) {
        return blocks;
    }

    string line;
    string currentBlock;

    while (getline(in, line)) {
        if (line.empty()) {
            if (!currentBlock.empty()) {
                blocks.push_back(currentBlock);
                currentBlock.clear();
            }
        }
        else {
            if (!currentBlock.empty()) {
                currentBlock += "\n"; 
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

// Save story progress into text file "endingsTotal chapterCompleted"
static void SaveStoryProgressInternal(const StoryProgress& p)
{
    ofstream out(STORY_PROGRESS_FILE);
    if (!out.is_open()) {
        // If we cannot open save file, just skip saving
        return;
    }

    out << p.endingsTotal << " " << p.chapterCompleted;
}

// -------------------------
// Story system functions
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
    // Count this ending and save immediately
    progress.endingsTotal += 1;
    SaveStoryProgressInternal(progress);

	// First ever ending -> no story
    if (progress.endingsTotal <= 1) {
        story.active = false;
        story.blocks.clear();
        story.currentBlock = 0;
        story.chapterIndex = 0;
        story.fullyRead = false;
        return;
    }
    
	// Decide which chapter to show (next after last completed)
    int chapterToShow = progress.chapterCompleted + 1;

    // If we passed the last chapter -> no more story
    if (chapterToShow > MAX_STORY_FILES) {
        story.active = false;
        story.blocks.clear();
        story.currentBlock = 0;
        story.chapterIndex = 0;
        story.fullyRead = false;
        return;
    }

    // Build file name
    string fileName = string("story_text/death") + to_string(chapterToShow) + ".md";

    vector<string> blocks = LoadStoryBlocksFromMarkdown(fileName);
    if (blocks.empty()) {
        story.active = false;
        story.blocks.clear();
        story.currentBlock = 0;
        story.chapterIndex = 0;
        story.fullyRead = false;
        return;
    }

    // Initialize runtime story state
    story.active = true;
    story.blocks =  move(blocks);
    story.currentBlock = 1;             // Start from first block
    story.chapterIndex = chapterToShow;
    story.fullyRead = false;
}

void MarkChapterCompletedIfNeeded(StoryProgress& progress, StoryState& story)
{
    if (!story.active) return;
    if (!story.fullyRead) return;
    if (story.chapterIndex <= 0) return;

    // Only move forward if this chapter is new
    if (progress.chapterCompleted < story.chapterIndex) {
        progress.chapterCompleted = story.chapterIndex;
        SaveStoryProgressInternal(progress);
    }
}
