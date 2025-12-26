#pragma once

#include <string>
#include <vector>

namespace beater {

// Instrument: maps to drum samples
class Instrument {
public:
    Instrument() = default;
    Instrument(int id, const std::string& name);
    
    // Accessors
    int getId() const { return id_; }
    const std::string& getName() const { return name_; }
    float getGain() const { return gain_; }
    float getPan() const { return pan_; }
    const std::string& getSamplePath() const { return samplePath_; }
    
    // Mutators
    void setName(const std::string& name) { name_ = name; }
    void setGain(float gain) { gain_ = gain; }
    void setPan(float pan) { pan_ = pan; }
    void setSamplePath(const std::string& path) { samplePath_ = path; }
    
private:
    int id_ = 0;
    std::string name_ = "Instrument";
    float gain_ = 1.0f;      // 0.0 to 1.0+
    float pan_ = 0.0f;       // -1.0 (left) to +1.0 (right)
    std::string samplePath_; // Path to WAV/sample file
};

// Instrument rack: collection of instruments in a project
class InstrumentRack {
public:
    InstrumentRack() = default;
    
    // Add/remove instruments
    void addInstrument(const Instrument& instrument);
    void removeInstrument(int id);
    Instrument* getInstrument(int id);
    const Instrument* getInstrument(int id) const;
    
    // Access all instruments
    const std::vector<Instrument>& getInstruments() const { return instruments_; }
    
    // Check if instrument exists
    bool hasInstrument(int id) const;
    
    // Clear all instruments
    void clear();
    
    // Get next available instrument ID
    int getNextId() const;
    
private:
    std::vector<Instrument> instruments_;
};

} // namespace beater
