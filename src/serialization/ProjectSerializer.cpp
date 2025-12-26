#include "serialization/ProjectSerializer.hpp"
#include "domain/Track.hpp"
#include "domain/Region.hpp"
#include "domain/Pattern.hpp"
#include "domain/Instrument.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

namespace beater {

// Helper function to serialize Pattern
json serializePattern(const Pattern& pattern) {
    json j;
    j["id"] = pattern.getId();
    j["name"] = pattern.getName();
    j["lengthTicks"] = pattern.getLengthTicks();
    
    json notes = json::array();
    for (const auto& note : pattern.getNotes()) {
        json noteJson;
        noteJson["instrumentId"] = note.instrumentId;
        noteJson["offsetTick"] = note.offsetTick;
        noteJson["velocity"] = note.velocity;
        noteJson["probability"] = note.probability;
        notes.push_back(noteJson);
    }
    j["notes"] = notes;
    
    return j;
}

// Helper function to deserialize Pattern
Pattern deserializePattern(const json& j) {
    Pattern pattern(
        j["id"].get<std::string>(),
        j["name"].get<std::string>(),
        j["lengthTicks"].get<Tick>()
    );
    
    for (const auto& noteJson : j["notes"]) {
        StepNote note;
        note.instrumentId = noteJson["instrumentId"].get<int>();
        note.offsetTick = noteJson["offsetTick"].get<Tick>();
        note.velocity = noteJson["velocity"].get<float>();
        note.probability = noteJson.value("probability", 1.0f);
        pattern.addNote(note);
    }
    
    return pattern;
}

// Helper function to serialize Region
json serializeRegion(const Region& region) {
    json j;
    j["id"] = region.getId();
    j["type"] = static_cast<int>(region.getType());
    j["startTick"] = region.getStartTick();
    j["lengthTicks"] = region.getLengthTicks();
    j["patternId"] = region.getPatternId();
    return j;
}

// Helper function to deserialize Region
Region deserializeRegion(const json& j) {
    Region region(
        j["id"].get<std::string>(),
        static_cast<RegionType>(j["type"].get<int>()),
        j["startTick"].get<Tick>(),
        j["lengthTicks"].get<Tick>()
    );
    region.setPatternId(j["patternId"].get<std::string>());
    return region;
}

// Helper function to serialize Track
json serializeTrack(const Track& track) {
    json j;
    j["id"] = track.getId();
    j["name"] = track.getName();
    
    json regions = json::array();
    for (const auto& region : track.getRegions()) {
        regions.push_back(serializeRegion(region));
    }
    j["regions"] = regions;
    
    return j;
}

// Helper function to deserialize Track
Track deserializeTrack(const json& j) {
    Track track(j["id"].get<std::string>(), j["name"].get<std::string>());
    
    for (const auto& regionJson : j["regions"]) {
        track.addRegion(deserializeRegion(regionJson));
    }
    
    return track;
}

// Helper function to serialize Instrument
json serializeInstrument(const Instrument& instrument) {
    json j;
    j["id"] = instrument.getId();
    j["name"] = instrument.getName();
    j["gain"] = instrument.getGain();
    j["pan"] = instrument.getPan();
    j["samplePath"] = instrument.getSamplePath();
    return j;
}

// Helper function to deserialize Instrument
Instrument deserializeInstrument(const json& j) {
    Instrument instrument(
        j["id"].get<int>(),
        j["name"].get<std::string>()
    );
    instrument.setGain(j["gain"].get<float>());
    instrument.setPan(j["pan"].get<float>());
    instrument.setSamplePath(j["samplePath"].get<std::string>());
    return instrument;
}

bool ProjectSerializer::saveToFile(const Project& project, const std::string& filepath) {
    try {
        json j;
        j["version"] = 1;
        j["name"] = project.getName();
        
        // Serialize patterns
        json patterns = json::array();
        const auto& patternLib = project.getPatternLibrary();
        for (const auto& pattern : patternLib.getPatterns()) {
            patterns.push_back(serializePattern(pattern));
        }
        j["patterns"] = patterns;
        
        // Serialize tracks
        json tracks = json::array();
        for (size_t i = 0; i < project.getTrackCount(); ++i) {
            const Track* track = project.getTrack(i);
            if (track) {
                tracks.push_back(serializeTrack(*track));
            }
        }
        j["tracks"] = tracks;
        
        // Serialize instruments
        json instruments = json::array();
        const auto& rack = project.getInstrumentRack();
        for (const auto& instrument : rack.getInstruments()) {
            instruments.push_back(serializeInstrument(instrument));
        }
        j["instruments"] = instruments;
        
        // Serialize meter map (time signatures)
        json meterChanges = json::array();
        // Note: MeterMap doesn't expose its changes, so we'd need to add that API
        // For now, we'll serialize the initial signature
        const auto& meterMap = project.getMeterMap();
        json initialSig;
        initialSig["tick"] = 0;
        initialSig["numerator"] = meterMap.getSignatureAt(0).numerator;
        initialSig["denominator"] = meterMap.getSignatureAt(0).denominator;
        meterChanges.push_back(initialSig);
        j["meterChanges"] = meterChanges;
        
        // Write to file
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for writing: " << filepath << std::endl;
            return false;
        }
        
        file << j.dump(2); // Pretty print with 2-space indent
        file.close();
        
        std::cout << "Project saved to: " << filepath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error saving project: " << e.what() << std::endl;
        return false;
    }
}

bool ProjectSerializer::loadFromFile(Project& project, const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Failed to open file for reading: " << filepath << std::endl;
            return false;
        }
        
        json j;
        file >> j;
        file.close();
        
        // Check version
        int version = j.value("version", 0);
        if (version != 1) {
            std::cerr << "Unsupported project version: " << version << std::endl;
            return false;
        }
        
        // Clear existing project data
        // Note: Project doesn't have a clear() method, so we'd need to add that
        // For now, we'll just add to existing data
        
        // Load patterns
        if (j.contains("patterns")) {
            for (const auto& patternJson : j["patterns"]) {
                Pattern pattern = deserializePattern(patternJson);
                project.getPatternLibrary().addPattern(pattern);
            }
        }
        
        // Load instruments
        if (j.contains("instruments")) {
            for (const auto& instrumentJson : j["instruments"]) {
                Instrument instrument = deserializeInstrument(instrumentJson);
                project.getInstrumentRack().addInstrument(instrument);
            }
        }
        
        // Load tracks
        if (j.contains("tracks")) {
            for (const auto& trackJson : j["tracks"]) {
                Track track = deserializeTrack(trackJson);
                project.addTrack(track);
            }
        }
        
        // Load meter map
        if (j.contains("meterChanges")) {
            for (const auto& changeJson : j["meterChanges"]) {
                Tick tick = changeJson["tick"].get<Tick>();
                TimeSignature sig{
                    changeJson["numerator"].get<int>(),
                    changeJson["denominator"].get<int>()
                };
                project.getMeterMap().addChange(tick, sig);
            }
        }
        
        std::cout << "Project loaded from: " << filepath << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error loading project: " << e.what() << std::endl;
        return false;
    }
}

} // namespace beater
