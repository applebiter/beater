#include "domain/Project.hpp"
#include <algorithm>

namespace beater {

Project::Project() {
    createDefault();
}

Project::Project(const std::string& name)
    : name_(name) {
    createDefault();
}

void Project::addTrack(const Track& track) {
    tracks_.push_back(track);
}

void Project::removeTrack(const std::string& trackId) {
    tracks_.erase(
        std::remove_if(tracks_.begin(), tracks_.end(),
            [&trackId](const Track& t) { return t.getId() == trackId; }),
        tracks_.end()
    );
}

Track* Project::getTrack(const std::string& trackId) {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
        [&trackId](const Track& t) { return t.getId() == trackId; });
    
    return (it != tracks_.end()) ? &(*it) : nullptr;
}

const Track* Project::getTrack(const std::string& trackId) const {
    auto it = std::find_if(tracks_.begin(), tracks_.end(),
        [&trackId](const Track& t) { return t.getId() == trackId; });
    
    return (it != tracks_.end()) ? &(*it) : nullptr;
}

Track* Project::getTrack(size_t index) {
    return (index < tracks_.size()) ? &tracks_[index] : nullptr;
}

const Track* Project::getTrack(size_t index) const {
    return (index < tracks_.size()) ? &tracks_[index] : nullptr;
}

void Project::clear() {
    name_ = "Untitled";
    revision_ = 0;
    tempoMap_.setConstantTempo(120.0);
    meterMap_.setConstantMeter({4, 4});
    patterns_.clear();
    instruments_.clear();
    tracks_.clear();
}

void Project::createDefault() {
    // Set default tempo and meter
    tempoMap_.setConstantTempo(120.0);
    meterMap_.setConstantMeter({4, 4});
    
    // Create one default track
    Track defaultTrack("track_0", "Drums");
    tracks_.push_back(defaultTrack);
    
    // Create default instruments (kick, snare, hi-hat)
    Instrument kick(1, "Kick");
    Instrument snare(2, "Snare");
    Instrument hihat(3, "Hi-Hat");
    
    instruments_.addInstrument(kick);
    instruments_.addInstrument(snare);
    instruments_.addInstrument(hihat);
    
    revision_ = 0;
}

} // namespace beater
