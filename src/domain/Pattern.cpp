#include "domain/Pattern.hpp"
#include <algorithm>

namespace beater {

Pattern::Pattern(const std::string& id, const std::string& name, Tick lengthTicks)
    : id_(id), name_(name), lengthTicks_(lengthTicks) {
}

void Pattern::addNote(const StepNote& note) {
    notes_.push_back(note);
    
    // Keep notes sorted by tick for efficient playback
    std::sort(notes_.begin(), notes_.end(), 
        [](const StepNote& a, const StepNote& b) {
            return a.offsetTick < b.offsetTick;
        });
}

void Pattern::removeNote(size_t index) {
    if (index < notes_.size()) {
        notes_.erase(notes_.begin() + index);
    }
}

void Pattern::clearNotes() {
    notes_.clear();
}

std::vector<StepNote> Pattern::getNotesAt(Tick tick) const {
    std::vector<StepNote> result;
    for (const auto& note : notes_) {
        if (note.offsetTick == tick) {
            result.push_back(note);
        }
    }
    return result;
}

std::vector<StepNote> Pattern::getNotesForInstrument(int instrumentId) const {
    std::vector<StepNote> result;
    for (const auto& note : notes_) {
        if (note.instrumentId == instrumentId) {
            result.push_back(note);
        }
    }
    return result;
}

// PatternLibrary implementation

void PatternLibrary::addPattern(const Pattern& pattern) {
    // Remove existing pattern with same ID if present
    removePattern(pattern.getId());
    patterns_.push_back(pattern);
}

void PatternLibrary::removePattern(const std::string& id) {
    patterns_.erase(
        std::remove_if(patterns_.begin(), patterns_.end(),
            [&id](const Pattern& p) { return p.getId() == id; }),
        patterns_.end()
    );
}

Pattern* PatternLibrary::getPattern(const std::string& id) {
    auto it = std::find_if(patterns_.begin(), patterns_.end(),
        [&id](const Pattern& p) { return p.getId() == id; });
    
    return (it != patterns_.end()) ? &(*it) : nullptr;
}

const Pattern* PatternLibrary::getPattern(const std::string& id) const {
    auto it = std::find_if(patterns_.begin(), patterns_.end(),
        [&id](const Pattern& p) { return p.getId() == id; });
    
    return (it != patterns_.end()) ? &(*it) : nullptr;
}

bool PatternLibrary::hasPattern(const std::string& id) const {
    return getPattern(id) != nullptr;
}

void PatternLibrary::clear() {
    patterns_.clear();
}

} // namespace beater
