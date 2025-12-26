#pragma once

#include "domain/Pattern.hpp"
#include "domain/Instrument.hpp"
#include "domain/Project.hpp"
#include "domain/TimeTypes.hpp"
#include "engine/Transport.hpp"
#include <vector>
#include <memory>

namespace beater {

// Compiled event ready for scheduling
struct CompiledEvent {
    Tick tick;              // Absolute tick position
    int instrumentId;
    float velocity;
    
    bool operator<(const CompiledEvent& other) const {
        return tick < other.tick;
    }
};

// Scheduler: generates sample triggers from timeline arrangement
class Scheduler {
public:
    Scheduler();
    
    // Phase 4: Set project for timeline-based playback
    void setProject(const Project* project);
    
    // Phase 3 compatibility: Set single pattern for simple loop playback
    void setPattern(const Pattern* pattern);
    void setLoopLength(Tick ticks) { loopLengthTicks_ = ticks; }
    void setLooping(bool enabled) { looping_ = enabled; }
    
    // Get events for the current cycle
    // Phase 4: Queries all active regions across all tracks
    // Phase 3: Returns events from single looping pattern
    std::vector<CompiledEvent> getEventsInRange(Tick startTick, Tick endTick);
    
    // Clear scheduler state
    void clear();
    
private:
    // Phase 4: Timeline mode
    const Project* project_ = nullptr;
    
    // Phase 3: Single pattern mode (legacy)
    const Pattern* pattern_ = nullptr;
    Tick loopLengthTicks_ = 0;
    bool looping_ = true;
    
    // Phase 4 helpers
    std::vector<CompiledEvent> getEventsFromTimeline(Tick startTick, Tick endTick);
    std::vector<CompiledEvent> getEventsFromRegion(const Region& region, 
                                                     const Pattern* pattern,
                                                     Tick startTick, 
                                                     Tick endTick);
    
    // Phase 3 helper
    std::vector<CompiledEvent> getEventsFromSinglePattern(Tick startTick, Tick endTick);
};

} // namespace beater
