#include "engine/Transport.hpp"
#include <cstring>

namespace beater {

Transport::Transport() {
}

void Transport::updateFromJack(const jack_position_t& jackPos,
                               jack_transport_state_t jackState,
                               uint32_t sampleRate) {
    state_.rolling = (jackState == JackTransportRolling);
    state_.frame = jackPos.frame;
    state_.sampleRate = sampleRate;
    
    // Get tempo and time signature from JACK if available
    if (jackPos.valid & JackPositionBBT) {
        state_.bpm = jackPos.beats_per_minute;
        state_.signature.numerator = jackPos.beats_per_bar;
        state_.signature.denominator = jackPos.beat_type;
        
        // Calculate tick from BBT
        // bar (0-based) * ticks_per_bar + beat (0-based) * ticks_per_beat + tick
        Tick ticksPerBar = TimeUtils::ticksPerBar(state_.signature);
        Tick ticksPerBeat = TimeUtils::ticksPerBeat(state_.signature);
        
        int bar = jackPos.bar - 1;  // JACK bars are 1-based
        int beat = jackPos.beat - 1; // JACK beats are 1-based
        int tick = static_cast<int>(jackPos.tick);
        
        state_.tick = bar * ticksPerBar + beat * ticksPerBeat + tick;
    } else {
        // No BBT info, calculate tick from frames
        state_.tick = frameToTick(state_.frame, state_.bpm, sampleRate);
    }
}

void Transport::updateInternal(uint32_t nframes, uint32_t sampleRate) {
    if (!state_.rolling) {
        return;
    }
    
    state_.sampleRate = sampleRate;
    state_.frame += nframes;
    
    // Update tick based on frame position
    state_.tick = frameToTick(state_.frame, state_.bpm, sampleRate);
}

void Transport::setPosition(Tick tick) {
    state_.tick = tick;
    state_.frame = tickToFrame(tick, state_.bpm, state_.sampleRate);
}

void Transport::setTempo(double bpm) {
    state_.bpm = bpm;
}

Tick Transport::frameToTick(uint64_t frame, double bpm, uint32_t sampleRate) const {
    return TimeUtils::framesToTicks(frame, bpm, sampleRate);
}

uint64_t Transport::tickToFrame(Tick tick, double bpm, uint32_t sampleRate) const {
    return TimeUtils::ticksToFrames(tick, bpm, sampleRate);
}

} // namespace beater
