#include "domain/MeterMap.hpp"
#include <algorithm>

namespace beater {

MeterMap::MeterMap() {
    // Default: 4/4 at start
    changes_.push_back({0, {4, 4}});
}

MeterMap::MeterMap(const TimeSignature& initialSignature) {
    changes_.push_back({0, initialSignature});
}

void MeterMap::addChange(Tick atTick, const TimeSignature& signature) {
    // Remove any existing change at this exact tick
    removeChangeAt(atTick);
    
    changes_.push_back({atTick, signature});
    sortChanges();
}

void MeterMap::removeChangeAt(Tick tick) {
    changes_.erase(
        std::remove_if(changes_.begin(), changes_.end(),
            [tick](const MeterChange& mc) { return mc.atTick == tick; }),
        changes_.end()
    );
}

TimeSignature MeterMap::getSignatureAt(Tick tick) const {
    if (changes_.empty()) {
        return {4, 4}; // fallback
    }
    
    // Find the last meter change at or before this tick
    TimeSignature sig = changes_[0].signature;
    for (const auto& change : changes_) {
        if (change.atTick <= tick) {
            sig = change.signature;
        } else {
            break;
        }
    }
    
    return sig;
}

void MeterMap::clear() {
    changes_.clear();
}

void MeterMap::setConstantMeter(const TimeSignature& signature) {
    changes_.clear();
    changes_.push_back({0, signature});
}

Tick MeterMap::getBarStartAt(Tick tick) const {
    if (changes_.empty()) {
        TimeSignature defaultSig = {4, 4};
        Tick barLength = TimeUtils::ticksPerBar(defaultSig);
        return (tick / barLength) * barLength;
    }
    
    // Walk through meter changes to find which bar we're in
    Tick currentTick = 0;
    TimeSignature currentSig = changes_[0].signature;
    
    for (size_t i = 0; i < changes_.size(); ++i) {
        const auto& change = changes_[i];
        
        // Determine the range where this signature applies
        Tick rangeStart = change.atTick;
        Tick rangeEnd = (i + 1 < changes_.size()) ? changes_[i + 1].atTick : tick + 1;
        
        if (tick < rangeStart) {
            break;
        }
        
        if (tick >= rangeStart && tick < rangeEnd) {
            // Tick is within this meter section
            currentSig = change.signature;
            Tick barLength = TimeUtils::ticksPerBar(currentSig);
            Tick offsetInSection = tick - rangeStart;
            Tick barStartInSection = (offsetInSection / barLength) * barLength;
            return rangeStart + barStartInSection;
        }
    }
    
    // Fallback: use last known signature
    if (!changes_.empty()) {
        currentSig = changes_.back().signature;
        Tick barLength = TimeUtils::ticksPerBar(currentSig);
        return (tick / barLength) * barLength;
    }
    
    return 0;
}

int MeterMap::getBarIndexAt(Tick tick) const {
    if (changes_.empty()) {
        TimeSignature defaultSig = {4, 4};
        Tick barLength = TimeUtils::ticksPerBar(defaultSig);
        return static_cast<int>(tick / barLength);
    }
    
    // Count bars across meter changes
    int barCount = 0;
    Tick currentTick = 0;
    
    for (size_t i = 0; i < changes_.size(); ++i) {
        const auto& change = changes_[i];
        Tick rangeStart = change.atTick;
        Tick rangeEnd = (i + 1 < changes_.size()) ? changes_[i + 1].atTick : tick + 1;
        
        if (tick < rangeStart) {
            break;
        }
        
        TimeSignature sig = change.signature;
        Tick barLength = TimeUtils::ticksPerBar(sig);
        
        Tick effectiveEnd = (tick < rangeEnd) ? tick : rangeEnd;
        Tick ticksInSection = effectiveEnd - rangeStart;
        int barsInSection = static_cast<int>(ticksInSection / barLength);
        
        barCount += barsInSection;
        
        if (tick < rangeEnd) {
            break;
        }
    }
    
    return barCount;
}

void MeterMap::sortChanges() {
    std::sort(changes_.begin(), changes_.end());
}

} // namespace beater
