#include "domain/TimeTypes.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>

namespace beater {

std::string MusicalPosition::toString() const {
    std::ostringstream oss;
    oss << (bar + 1) << ":" << (beat + 1) << ":" << std::setfill('0') << std::setw(3) << tick;
    return oss.str();
}

Tick TimeUtils::ticksPerBar(const TimeSignature& ts) {
    // Bar length in ticks = (numerator / denominator) * 4 * PPQ
    // For 4/4: (4/4) * 4 * 960 = 3840
    // For 3/4: (3/4) * 4 * 960 = 2880
    return (ts.numerator * 4 * PPQ) / ts.denominator;
}

Tick TimeUtils::ticksPerBeat(const TimeSignature& ts) {
    // Beat is defined by the denominator
    // For x/4 time: beat = quarter note = PPQ
    // For x/8 time: beat = eighth note = PPQ/2
    return (4 * PPQ) / ts.denominator;
}

MusicalPosition TimeUtils::tickToPosition(Tick tick, const TimeSignature& ts) {
    MusicalPosition pos;
    
    Tick barLength = ticksPerBar(ts);
    Tick beatLength = ticksPerBeat(ts);
    
    pos.bar = static_cast<int>(tick / barLength);
    Tick remainder = tick % barLength;
    
    pos.beat = static_cast<int>(remainder / beatLength);
    pos.tick = remainder % beatLength;
    
    return pos;
}

Tick TimeUtils::positionToTick(const MusicalPosition& pos, const TimeSignature& ts) {
    Tick barLength = ticksPerBar(ts);
    Tick beatLength = ticksPerBeat(ts);
    
    return pos.bar * barLength + pos.beat * beatLength + pos.tick;
}

Tick TimeUtils::snapToBar(Tick tick, const TimeSignature& ts) {
    Tick barLength = ticksPerBar(ts);
    Tick remainder = tick % barLength;
    
    // Round to nearest bar
    if (remainder < barLength / 2) {
        return tick - remainder;
    } else {
        return tick + (barLength - remainder);
    }
}

Tick TimeUtils::snapToBeat(Tick tick, const TimeSignature& ts) {
    Tick beatLength = ticksPerBeat(ts);
    Tick remainder = tick % beatLength;
    
    // Round to nearest beat
    if (remainder < beatLength / 2) {
        return tick - remainder;
    } else {
        return tick + (beatLength - remainder);
    }
}

Tick TimeUtils::snapToGrid(Tick tick, int subdivision) {
    // subdivision: 1 = quarter, 2 = eighth, 4 = sixteenth
    Tick gridSize = PPQ / subdivision;
    Tick remainder = tick % gridSize;
    
    // Round to nearest grid line
    if (remainder < gridSize / 2) {
        return tick - remainder;
    } else {
        return tick + (gridSize - remainder);
    }
}

uint64_t TimeUtils::ticksToFrames(Tick ticks, double bpm, uint32_t sampleRate) {
    // At tempo bpm, one quarter note takes (60.0 / bpm) seconds
    // One quarter = PPQ ticks
    // frames per tick = (sampleRate * 60.0) / (bpm * PPQ)
    double framesPerTickValue = framesPerTick(bpm, sampleRate);
    return static_cast<uint64_t>(std::round(ticks * framesPerTickValue));
}

Tick TimeUtils::framesToTicks(uint64_t frames, double bpm, uint32_t sampleRate) {
    double framesPerTickValue = framesPerTick(bpm, sampleRate);
    return static_cast<Tick>(std::round(frames / framesPerTickValue));
}

double TimeUtils::framesPerTick(double bpm, uint32_t sampleRate) {
    // frames per tick = (sampleRate * 60.0) / (bpm * PPQ)
    return (sampleRate * 60.0) / (bpm * PPQ);
}

} // namespace beater
