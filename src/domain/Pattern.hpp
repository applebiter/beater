#pragma once

#include "domain/TimeTypes.hpp"
#include <string>
#include <vector>

namespace beater {

// A note hit within a pattern
struct StepNote {
    int instrumentId = 0;       // Which instrument to trigger
    Tick offsetTick = 0;        // Tick offset within pattern (0-based)
    float velocity = 0.8f;      // 0.0 to 1.0
    float probability = 1.0f;   // 0.0 to 1.0 (for future humanization)
    
    bool operator==(const StepNote& other) const {
        return instrumentId == other.instrumentId &&
               offsetTick == other.offsetTick &&
               velocity == other.velocity &&
               probability == other.probability;
    }
};

// Pattern: a reusable sequence of note events (Hydrogen-like)
class Pattern {
public:
    Pattern() = default;
    Pattern(const std::string& id, const std::string& name, Tick lengthTicks);
    
    // Accessors
    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    Tick getLengthTicks() const { return lengthTicks_; }
    const std::vector<StepNote>& getNotes() const { return notes_; }
    
    // Mutators
    void setName(const std::string& name) { name_ = name; }
    void setLengthTicks(Tick ticks) { lengthTicks_ = ticks; }
    
    // Note manipulation
    void addNote(const StepNote& note);
    void removeNote(size_t index);
    void clearNotes();
    
    // Find notes at a specific tick
    std::vector<StepNote> getNotesAt(Tick tick) const;
    
    // Get all notes for a specific instrument
    std::vector<StepNote> getNotesForInstrument(int instrumentId) const;
    
private:
    std::string id_;
    std::string name_;
    Tick lengthTicks_ = PPQ * 4; // Default: 1 bar in 4/4
    std::vector<StepNote> notes_;
};

// Pattern library: collection of reusable patterns
class PatternLibrary {
public:
    PatternLibrary() = default;
    
    // Add/remove patterns
    void addPattern(const Pattern& pattern);
    void removePattern(const std::string& id);
    Pattern* getPattern(const std::string& id);
    const Pattern* getPattern(const std::string& id) const;
    
    // Access all patterns
    const std::vector<Pattern>& getPatterns() const { return patterns_; }
    
    // Check if pattern exists
    bool hasPattern(const std::string& id) const;
    
    // Clear all patterns
    void clear();
    
private:
    std::vector<Pattern> patterns_;
};

} // namespace beater
