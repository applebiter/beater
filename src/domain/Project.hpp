#pragma once

#include "domain/Track.hpp"
#include "domain/Pattern.hpp"
#include "domain/Instrument.hpp"
#include "domain/TempoMap.hpp"
#include "domain/MeterMap.hpp"
#include <string>
#include <vector>

namespace beater {

// Project: top-level document containing all musical data
class Project {
public:
    Project();
    explicit Project(const std::string& name);
    
    // Project metadata
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    uint64_t getRevision() const { return revision_; }
    void incrementRevision() { ++revision_; }
    
    // Tempo and meter
    TempoMap& getTempoMap() { return tempoMap_; }
    const TempoMap& getTempoMap() const { return tempoMap_; }
    
    MeterMap& getMeterMap() { return meterMap_; }
    const MeterMap& getMeterMap() const { return meterMap_; }
    
    // Patterns
    PatternLibrary& getPatternLibrary() { return patterns_; }
    const PatternLibrary& getPatternLibrary() const { return patterns_; }
    
    // Instruments
    InstrumentRack& getInstrumentRack() { return instruments_; }
    const InstrumentRack& getInstrumentRack() const { return instruments_; }
    
    // Tracks
    void addTrack(const Track& track);
    void removeTrack(const std::string& trackId);
    Track* getTrack(const std::string& trackId);
    const Track* getTrack(const std::string& trackId) const;
    Track* getTrack(size_t index);
    const Track* getTrack(size_t index) const;
    
    const std::vector<Track>& getTracks() const { return tracks_; }
    size_t getTrackCount() const { return tracks_.size(); }
    
    // Clear project data
    void clear();
    
    // Create a default empty project with one track
    void createDefault();
    
private:
    std::string name_ = "Untitled";
    uint64_t revision_ = 0;
    
    TempoMap tempoMap_;
    MeterMap meterMap_;
    PatternLibrary patterns_;
    InstrumentRack instruments_;
    std::vector<Track> tracks_;
};

} // namespace beater
