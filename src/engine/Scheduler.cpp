#include "engine/Scheduler.hpp"
#include <algorithm>
#include <iostream>

namespace beater {

Scheduler::Scheduler() {
}

void Scheduler::setProject(const Project* project) {
    project_ = project;
    pattern_ = nullptr;  // Clear legacy mode
}

void Scheduler::setPattern(const Pattern* pattern) {
    pattern_ = pattern;
    project_ = nullptr;  // Clear timeline mode
    if (pattern_ && loopLengthTicks_ == 0) {
        loopLengthTicks_ = pattern_->getLengthTicks();
    }
}

void Scheduler::clear() {
    pattern_ = nullptr;
    project_ = nullptr;
}

std::vector<CompiledEvent> Scheduler::getEventsInRange(Tick startTick, Tick endTick) {
    // Phase 4: Use timeline if project is set
    if (project_ != nullptr) {
        return getEventsFromTimeline(startTick, endTick);
    }
    
    // Phase 3: Fall back to single pattern mode
    if (pattern_ != nullptr) {
        return getEventsFromSinglePattern(startTick, endTick);
    }
    
    return {};
}

std::vector<CompiledEvent> Scheduler::getEventsFromTimeline(Tick startTick, Tick endTick) {
    std::vector<CompiledEvent> allEvents;
    
    // Iterate through all tracks
    for (const auto& track : project_->getTracks()) {
        // Get regions that overlap with [startTick, endTick)
        auto regions = track.getRegionsInRange(startTick, endTick);
        
        for (const Region* region : regions) {
            // Get the pattern for this region
            const Pattern* pattern = project_->getPatternLibrary().getPattern(region->getPatternId());
            if (pattern == nullptr) {
                continue;
            }
            
            // Get events from this region
            auto regionEvents = getEventsFromRegion(*region, pattern, startTick, endTick);
            
            // Merge into all events
            allEvents.insert(allEvents.end(), regionEvents.begin(), regionEvents.end());
        }
    }
    
    // Sort by tick
    std::sort(allEvents.begin(), allEvents.end());
    
    return allEvents;
}

std::vector<CompiledEvent> Scheduler::getEventsFromRegion(const Region& region,
                                                            const Pattern* pattern,
                                                            Tick startTick,
                                                            Tick endTick) {
    std::vector<CompiledEvent> events;
    
    Tick regionStart = region.getStartTick();
    Tick regionEnd = regionStart + region.getLengthTicks();
    
    // Only process if region overlaps with query range
    if (regionEnd <= startTick || regionStart >= endTick) {
        return events;
    }
    
    Tick patternLength = pattern->getLengthTicks();
    if (patternLength == 0) {
        return events;
    }
    
    // Calculate how many times the pattern repeats in this region
    Tick regionLength = region.getLengthTicks();
    int numRepeats = (regionLength + patternLength - 1) / patternLength;
    
    // Generate events for each pattern repetition within the region
    for (int rep = 0; rep < numRepeats; ++rep) {
        Tick repeatStart = regionStart + (rep * patternLength);
        Tick repeatEnd = repeatStart + patternLength;
        
        // Skip if this repeat is outside query range
        if (repeatEnd <= startTick || repeatStart >= endTick) {
            continue;
        }
        
        // Add pattern notes, offset by repeat position
        for (const auto& note : pattern->getNotes()) {
            Tick eventTick = repeatStart + note.offsetTick;
            
            // Only include events in the requested range and within region bounds
            if (eventTick >= startTick && eventTick < endTick && 
                eventTick >= regionStart && eventTick < regionEnd) {
                CompiledEvent event;
                event.tick = eventTick;
                event.instrumentId = note.instrumentId;
                event.velocity = note.velocity;
                events.push_back(event);
            }
        }
    }
    
    return events;
}

std::vector<CompiledEvent> Scheduler::getEventsFromSinglePattern(Tick startTick, Tick endTick) {
    std::vector<CompiledEvent> events;
    
    if (pattern_ == nullptr || pattern_->getNotes().empty()) {
        return events;
    }
    
    if (loopLengthTicks_ == 0) {
        return events;
    }
    
    // For looping pattern: generate events across all loop iterations in range
    if (looping_) {
        // Calculate which loop iterations overlap with [startTick, endTick)
        Tick loopStart = (startTick / loopLengthTicks_) * loopLengthTicks_;
        Tick loopEnd = ((endTick + loopLengthTicks_ - 1) / loopLengthTicks_) * loopLengthTicks_;
        
        // Generate events for each loop iteration
        for (Tick loopTick = loopStart; loopTick < loopEnd; loopTick += loopLengthTicks_) {
            // Add all pattern notes offset by loop position
            for (const auto& note : pattern_->getNotes()) {
                Tick eventTick = loopTick + note.offsetTick;
                
                // Only include events in the requested range
                if (eventTick >= startTick && eventTick < endTick) {
                    CompiledEvent event;
                    event.tick = eventTick;
                    event.instrumentId = note.instrumentId;
                    event.velocity = note.velocity;
                    events.push_back(event);
                }
            }
        }
    } else {
        // One-shot: play pattern once
        for (const auto& note : pattern_->getNotes()) {
            Tick eventTick = note.offsetTick;
            
            if (eventTick >= startTick && eventTick < endTick) {
                CompiledEvent event;
                event.tick = eventTick;
                event.instrumentId = note.instrumentId;
                event.velocity = note.velocity;
                events.push_back(event);
            }
        }
    }
    
    // Sort events by tick
    std::sort(events.begin(), events.end());
    
    return events;
}

} // namespace beater
