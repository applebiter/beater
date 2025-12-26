#include "engine/Sampler.hpp"
#include <algorithm>
#include <cmath>

namespace beater {

Sampler::Sampler() {
    // Initialize all voices as inactive
    for (auto& voice : voices_) {
        voice.reset();
    }
}

void Sampler::noteOn(std::shared_ptr<Sample> sample, float velocity,
                     float gain, float pan, uint32_t offsetFrames) {
    if (sample == nullptr || sample->lengthFrames == 0) {
        return;
    }
    
    Voice* voice = allocateVoice();
    if (voice == nullptr) {
        // No free voices available (voice stealing could be implemented here)
        return;
    }
    
    // Initialize voice
    voice->sample = sample;
    voice->playbackPosition = 0;
    voice->velocity = velocity;
    voice->gain = gain;
    voice->pan = pan;
    voice->active = true;
    
    // Note: offsetFrames is stored for sample-accurate triggering
    // For now, we trigger immediately at the start of the block
    (void)offsetFrames;  // Suppress unused warning for MVP
}

void Sampler::allNotesOff() {
    for (auto& voice : voices_) {
        voice.reset();
    }
}

void Sampler::render(float* outL, float* outR, uint32_t nframes) {
    // Render all active voices
    for (auto& voice : voices_) {
        if (voice.active) {
            renderVoice(voice, outL, outR, 0, nframes);
        }
    }
}

size_t Sampler::getActiveVoiceCount() const {
    size_t count = 0;
    for (const auto& voice : voices_) {
        if (voice.active) {
            ++count;
        }
    }
    return count;
}

Voice* Sampler::allocateVoice() {
    // Find first inactive voice
    for (auto& voice : voices_) {
        if (!voice.active) {
            return &voice;
        }
    }
    return nullptr;  // No free voices
}

void Sampler::renderVoice(Voice& voice, float* outL, float* outR,
                          uint32_t startFrame, uint32_t nframes) {
    if (!voice.active || voice.sample == nullptr) {
        return;
    }
    
    const auto& sample = voice.sample;
    const float* sampleL = sample->dataLeft.data();
    const float* sampleR = sample->dataRight.data();
    
    // Calculate pan gains
    float panL = 1.0f;
    float panR = 1.0f;
    if (voice.pan < 0.0f) {
        // Pan left: reduce right channel
        panR = 1.0f + voice.pan;
    } else if (voice.pan > 0.0f) {
        // Pan right: reduce left channel
        panL = 1.0f - voice.pan;
    }
    
    const float gainL = voice.velocity * voice.gain * panL;
    const float gainR = voice.velocity * voice.gain * panR;
    
    // Render frames
    for (uint32_t i = startFrame; i < nframes; ++i) {
        if (voice.playbackPosition >= sample->lengthFrames) {
            // Sample finished
            voice.reset();
            return;
        }
        
        const uint64_t pos = voice.playbackPosition;
        outL[i] += sampleL[pos] * gainL;
        outR[i] += sampleR[pos] * gainR;
        
        voice.playbackPosition++;
    }
}

} // namespace beater
