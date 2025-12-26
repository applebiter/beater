#pragma once

#include <cstdint>
#include <string>

namespace beater {

// Musical time in ticks (PPQ = 960 ticks per quarter note)
using Tick = int64_t;

constexpr int PPQ = 960; // Pulses Per Quarter note

// Time signature
struct TimeSignature {
    int numerator = 4;
    int denominator = 4;
    
    bool operator==(const TimeSignature& other) const {
        return numerator == other.numerator && denominator == other.denominator;
    }
    
    bool operator!=(const TimeSignature& other) const {
        return !(*this == other);
    }
};

// Musical position (bar:beat:tick)
struct MusicalPosition {
    int bar = 0;      // 0-based bar index
    int beat = 0;     // 0-based beat within bar
    Tick tick = 0;    // tick within beat
    
    std::string toString() const;
};

// Time conversion utilities
class TimeUtils {
public:
    // Get ticks in one bar for a given time signature
    static Tick ticksPerBar(const TimeSignature& ts);
    
    // Get ticks in one beat for a given time signature
    static Tick ticksPerBeat(const TimeSignature& ts);
    
    // Convert absolute tick to bar:beat:tick
    static MusicalPosition tickToPosition(Tick tick, const TimeSignature& ts);
    
    // Convert bar:beat:tick to absolute tick
    static Tick positionToTick(const MusicalPosition& pos, const TimeSignature& ts);
    
    // Snap tick to nearest bar boundary
    static Tick snapToBar(Tick tick, const TimeSignature& ts);
    
    // Snap tick to nearest beat
    static Tick snapToBeat(Tick tick, const TimeSignature& ts);
    
    // Snap tick to grid subdivision (1 = quarter, 2 = eighth, 4 = sixteenth, etc.)
    static Tick snapToGrid(Tick tick, int subdivision);
    
    // Convert ticks to frames at fixed tempo
    // bpm: beats per minute
    // sampleRate: audio sample rate
    static uint64_t ticksToFrames(Tick ticks, double bpm, uint32_t sampleRate);
    
    // Convert frames to ticks at fixed tempo
    static Tick framesToTicks(uint64_t frames, double bpm, uint32_t sampleRate);
    
    // Get frames per tick at fixed tempo
    static double framesPerTick(double bpm, uint32_t sampleRate);
};

} // namespace beater
