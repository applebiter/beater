#include "domain/TempoMap.hpp"
#include <algorithm>

namespace beater {

TempoMap::TempoMap() {
    // Default: 120 BPM at start
    changes_.push_back({0, 120.0});
}

TempoMap::TempoMap(double initialBpm) {
    changes_.push_back({0, initialBpm});
}

void TempoMap::addChange(Tick atTick, double bpm) {
    // Remove any existing change at this exact tick
    removeChangeAt(atTick);
    
    changes_.push_back({atTick, bpm});
    sortChanges();
}

void TempoMap::removeChangeAt(Tick tick) {
    changes_.erase(
        std::remove_if(changes_.begin(), changes_.end(),
            [tick](const TempoChange& tc) { return tc.atTick == tick; }),
        changes_.end()
    );
}

double TempoMap::getBpmAt(Tick tick) const {
    if (changes_.empty()) {
        return 120.0; // fallback
    }
    
    // Find the last tempo change at or before this tick
    double bpm = changes_[0].bpm;
    for (const auto& change : changes_) {
        if (change.atTick <= tick) {
            bpm = change.bpm;
        } else {
            break;
        }
    }
    
    return bpm;
}

void TempoMap::clear() {
    changes_.clear();
}

void TempoMap::setConstantTempo(double bpm) {
    changes_.clear();
    changes_.push_back({0, bpm});
}

void TempoMap::sortChanges() {
    std::sort(changes_.begin(), changes_.end());
}

} // namespace beater
