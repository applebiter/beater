#pragma once

#include "domain/TimeTypes.hpp"
#include "domain/TempoMap.hpp"
#include <jack/jack.h>
#include <cstdint>

namespace beater {

// Transport state
struct TransportState {
    bool rolling = false;
    uint64_t frame = 0;          // Current frame position
    Tick tick = 0;               // Current musical tick position
    double bpm = 120.0;
    TimeSignature signature = {4, 4};
    uint32_t sampleRate = 48000;
};

// Transport manager - handles timing and position
class Transport {
public:
    Transport();
    
    // Update from JACK transport
    void updateFromJack(const jack_position_t& jackPos, 
                       jack_transport_state_t jackState,
                       uint32_t sampleRate);
    
    // Update with internal transport (when not following JACK)
    void updateInternal(uint32_t nframes, uint32_t sampleRate);
    
    // Get current state
    const TransportState& getState() const { return state_; }
    
    // Transport controls (internal)
    void play() { state_.rolling = true; }
    void stop() { state_.rolling = false; }
    void setPosition(Tick tick);
    void setTempo(double bpm);
    
    // Convert between time domains
    Tick frameToTick(uint64_t frame, double bpm, uint32_t sampleRate) const;
    uint64_t tickToFrame(Tick tick, double bpm, uint32_t sampleRate) const;
    
    // Check if transport has advanced
    bool isRolling() const { return state_.rolling; }
    
private:
    TransportState state_;
};

} // namespace beater
