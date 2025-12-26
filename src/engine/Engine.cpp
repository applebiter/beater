#include "engine/Engine.hpp"
#include <functional>
#include <iostream>

namespace beater {

Engine::Engine() {
}

Engine::~Engine() {
    shutdown();
}

bool Engine::initialize(const std::string& clientName) {
    // Initialize JACK
    if (!audioBackend_.initialize(clientName)) {
        return false;
    }
    
    // Set up audio callback
    audioBackend_.setAudioCallback(
        [this](jack_nframes_t nframes, float* outL, float* outR) {
            this->audioCallback(nframes, outL, outR);
        }
    );
    
    return true;
}

void Engine::shutdown() {
    stopPlayback();
    audioBackend_.shutdown();
}

void Engine::triggerSample(std::shared_ptr<Sample> sample, float velocity,
                           float gain, float pan) {
    sampler_.noteOn(sample, velocity, gain, pan, 0);
}

void Engine::playPattern(const Pattern* pattern) {
    if (pattern == nullptr) {
        return;
    }
    
    std::cout << "Playing pattern: " << pattern->getName() 
              << " (" << pattern->getLengthTicks() << " ticks)\n";
    
    scheduler_.setPattern(pattern);
    scheduler_.setLoopLength(pattern->getLengthTicks());
    scheduler_.setLooping(true);
    
    // Reset transport to start
    transport_.setPosition(0);
    transport_.play();
    
    lastProcessedTick_ = 0;
}

void Engine::playTimeline() {
    std::cout << "Playing timeline from start\n";
    
    scheduler_.setProject(&project_);
    
    // Reset transport to start
    transport_.setPosition(0);
    transport_.play();
    
    lastProcessedTick_ = 0;
}

void Engine::playFromTick(Tick startTick) {
    std::cout << "Playing timeline from tick " << startTick << "\n";
    
    scheduler_.setProject(&project_);
    
    // Set transport position
    transport_.setPosition(startTick);
    transport_.play();
    
    lastProcessedTick_ = startTick;
}

void Engine::stopPlayback() {
    transport_.stop();
    scheduler_.clear();
    sampler_.allNotesOff();
}

bool Engine::loadInstrumentSamples() {
    instrumentSamples_.clear();
    
    const auto& instruments = project_.getInstrumentRack().getInstruments();
    
    for (const auto& instrument : instruments) {
        if (instrument.getSamplePath().empty()) {
            std::cerr << "Instrument " << instrument.getId() 
                     << " has no sample path\n";
            continue;
        }
        
        auto sample = sampleLibrary_.loadSample(instrument.getSamplePath());
        if (sample) {
            instrumentSamples_[instrument.getId()] = sample;
            std::cout << "Loaded sample for instrument " << instrument.getId()
                     << ": " << instrument.getName() << "\n";
        } else {
            std::cerr << "Failed to load sample for instrument " 
                     << instrument.getId() << "\n";
            return false;
        }
    }
    
    return true;
}

std::shared_ptr<Sample> Engine::getSampleForInstrument(int instrumentId) {
    auto it = instrumentSamples_.find(instrumentId);
    return (it != instrumentSamples_.end()) ? it->second : nullptr;
}

void Engine::audioCallback(jack_nframes_t nframes, float* outL, float* outR) {
    // Update transport (use internal transport for now, JACK sync in Phase 4)
    transport_.updateInternal(nframes, audioBackend_.getSampleRate());
    
    // Get current state
    const auto& state = transport_.getState();
    
    // If transport is rolling, schedule events
    if (state.rolling) {
        // Calculate tick range for this audio block
        Tick startTick = state.tick;
        
        // Estimate end tick based on frame advancement
        uint64_t endFrame = state.frame + nframes;
        Tick endTick = transport_.frameToTick(endFrame, state.bpm, state.sampleRate);
        
        // Get events in this range
        auto events = scheduler_.getEventsInRange(startTick, endTick);
        
        // Trigger events
        for (const auto& event : events) {
            auto sample = getSampleForInstrument(event.instrumentId);
            if (sample) {
                // Calculate frame offset within this block
                uint64_t eventFrame = transport_.tickToFrame(event.tick, state.bpm, state.sampleRate);
                uint32_t offsetFrames = 0;
                if (eventFrame >= state.frame && eventFrame < endFrame) {
                    offsetFrames = static_cast<uint32_t>(eventFrame - state.frame);
                }
                
                // Get instrument settings
                const auto* instrument = project_.getInstrumentRack().getInstrument(event.instrumentId);
                float gain = instrument ? instrument->getGain() : 1.0f;
                float pan = instrument ? instrument->getPan() : 0.0f;
                
                sampler_.noteOn(sample, event.velocity, gain, pan, offsetFrames);
            }
        }
        
        lastProcessedTick_ = endTick;
    }
    
    // Render sampler voices
    sampler_.render(outL, outR, nframes);
}

} // namespace beater
