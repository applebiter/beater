#pragma once

#include "domain/TimeTypes.hpp"
#include <vector>

namespace beater {

// Meter (time signature) change at a specific tick
struct MeterChange {
    Tick atTick = 0;
    TimeSignature signature;
    
    bool operator<(const MeterChange& other) const {
        return atTick < other.atTick;
    }
};

// Meter map: piecewise constant time signatures across timeline
class MeterMap {
public:
    MeterMap();
    explicit MeterMap(const TimeSignature& initialSignature);
    
    // Add a meter change
    void addChange(Tick atTick, const TimeSignature& signature);
    
    // Remove meter change at tick
    void removeChangeAt(Tick tick);
    
    // Get time signature at a specific tick
    TimeSignature getSignatureAt(Tick tick) const;
    
    // Get all meter changes (sorted by tick)
    const std::vector<MeterChange>& getChanges() const { return changes_; }
    
    // Clear all changes
    void clear();
    
    // Set constant meter (clears all changes, sets single initial signature)
    void setConstantMeter(const TimeSignature& signature);
    
    // Get the start tick of the bar containing the given tick
    Tick getBarStartAt(Tick tick) const;
    
    // Get bar index (0-based) at given tick
    int getBarIndexAt(Tick tick) const;
    
private:
    std::vector<MeterChange> changes_;
    
    void sortChanges();
};

} // namespace beater
