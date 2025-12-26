#pragma once

#include "domain/TimeTypes.hpp"
#include <vector>

namespace beater {

// Tempo change at a specific tick
struct TempoChange {
    Tick atTick = 0;
    double bpm = 120.0;
    
    bool operator<(const TempoChange& other) const {
        return atTick < other.atTick;
    }
};

// Tempo map: piecewise constant tempo across timeline
class TempoMap {
public:
    TempoMap();
    explicit TempoMap(double initialBpm);
    
    // Add a tempo change
    void addChange(Tick atTick, double bpm);
    
    // Remove tempo change at tick
    void removeChangeAt(Tick tick);
    
    // Get BPM at a specific tick
    double getBpmAt(Tick tick) const;
    
    // Get all tempo changes (sorted by tick)
    const std::vector<TempoChange>& getChanges() const { return changes_; }
    
    // Clear all changes
    void clear();
    
    // Set constant tempo (clears all changes, sets single initial tempo)
    void setConstantTempo(double bpm);
    
private:
    std::vector<TempoChange> changes_;
    
    void sortChanges();
};

} // namespace beater
