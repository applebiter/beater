#pragma once

#include "engine/SampleLibrary.hpp"
#include <memory>
#include <vector>
#include <array>

namespace beater {

// Maximum number of simultaneous voices (RT-safe fixed size)
constexpr size_t MAX_VOICES = 64;

// Voice state for sample playback
struct Voice {
    std::shared_ptr<Sample> sample;
    uint64_t playbackPosition = 0;  // Current position in sample
    float velocity = 1.0f;
    float gain = 1.0f;
    float pan = 0.0f;  // -1.0 (left) to +1.0 (right)
    bool active = false;
    
    void reset() {
        sample = nullptr;
        playbackPosition = 0;
        velocity = 1.0f;
        gain = 1.0f;
        pan = 0.0f;
        active = false;
    }
};

// Sampler engine with voice management
class Sampler {
public:
    Sampler();
    ~Sampler() = default;
    
    // Trigger a voice (sample-accurate within block)
    // offsetFrames: offset within the current audio block
    void noteOn(std::shared_ptr<Sample> sample, float velocity, 
                float gain, float pan, uint32_t offsetFrames = 0);
    
    // Stop all voices
    void allNotesOff();
    
    // Render audio for nframes
    // Mixes all active voices into outL/outR buffers
    void render(float* outL, float* outR, uint32_t nframes);
    
    // Get number of active voices
    size_t getActiveVoiceCount() const;
    
private:
    std::array<Voice, MAX_VOICES> voices_;
    
    // Find an available voice slot
    Voice* allocateVoice();
    
    // Render a single voice
    void renderVoice(Voice& voice, float* outL, float* outR, 
                     uint32_t startFrame, uint32_t nframes);
};

} // namespace beater
