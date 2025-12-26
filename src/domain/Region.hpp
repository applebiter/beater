#pragma once

#include "domain/TimeTypes.hpp"
#include <string>

namespace beater {

// Region type classification
enum class RegionType {
    Groove,     // Repeating pattern (main groove)
    Fill,       // Fill pattern (overrides groove when overlapping)
    Signature,  // Time signature change (future)
    Tempo       // Tempo change (future)
};

// How region responds to length changes
enum class StretchMode {
    Repeat,     // Repeat pattern to fill length
    Truncate,   // Cut pattern if shorter, repeat if longer
    VariantSelect // Choose different pattern variant (future)
};

// Region: a block on the timeline referencing a pattern
class Region {
public:
    Region() = default;
    Region(const std::string& id, RegionType type, Tick startTick, Tick lengthTicks);
    
    // Accessors
    const std::string& getId() const { return id_; }
    RegionType getType() const { return type_; }
    Tick getStartTick() const { return startTick_; }
    Tick getLengthTicks() const { return lengthTicks_; }
    Tick getEndTick() const { return startTick_ + lengthTicks_; }
    const std::string& getPatternId() const { return patternId_; }
    StretchMode getStretchMode() const { return stretchMode_; }
    bool getSnapToBars() const { return snapToBars_; }
    
    // Mutators
    void setStartTick(Tick tick) { startTick_ = tick; }
    void setLengthTicks(Tick ticks) { lengthTicks_ = ticks; }
    void setPatternId(const std::string& id) { patternId_ = id; }
    void setStretchMode(StretchMode mode) { stretchMode_ = mode; }
    void setSnapToBars(bool snap) { snapToBars_ = snap; }
    
    // Check if a tick falls within this region
    bool contains(Tick tick) const {
        return tick >= startTick_ && tick < getEndTick();
    }
    
    // Check if this region overlaps with another
    bool overlaps(const Region& other) const {
        return startTick_ < other.getEndTick() && other.getStartTick() < getEndTick();
    }
    
private:
    std::string id_;
    RegionType type_ = RegionType::Groove;
    Tick startTick_ = 0;
    Tick lengthTicks_ = PPQ * 4; // Default: 1 bar
    std::string patternId_;
    StretchMode stretchMode_ = StretchMode::Repeat;
    bool snapToBars_ = true; // MVP: always snap to bars
};

} // namespace beater
