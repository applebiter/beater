#pragma once

#include "engine/JackAudioBackend.hpp"
#include "engine/Sampler.hpp"
#include "engine/SampleLibrary.hpp"
#include "engine/Transport.hpp"
#include "engine/Scheduler.hpp"
#include "domain/Project.hpp"
#include <memory>
#include <unordered_map>

namespace beater {

// Main engine class coordinating audio components
class Engine {
public:
    Engine();
    ~Engine();
    
    // Initialize JACK and audio engine
    bool initialize(const std::string& clientName = "beater");
    
    // Shutdown
    void shutdown();
    
    // Check if engine is running
    bool isActive() const { return audioBackend_.isActive(); }
    
    // Get components
    JackAudioBackend& getAudioBackend() { return audioBackend_; }
    Sampler& getSampler() { return sampler_; }
    SampleLibrary& getSampleLibrary() { return sampleLibrary_; }
    Transport& getTransport() { return transport_; }
    Scheduler& getScheduler() { return scheduler_; }
    
    // Project management
    void setProject(const Project& project) { project_ = project; }
    const Project& getProject() const { return project_; }
    Project& getProject() { return project_; }
    
    // Get audio info
    uint32_t getSampleRate() const { return audioBackend_.getSampleRate(); }
    uint32_t getBufferSize() const { return audioBackend_.getBufferSize(); }
    
    // Manual trigger for testing (Phase 2)
    void triggerSample(std::shared_ptr<Sample> sample, float velocity = 0.8f,
                      float gain = 1.0f, float pan = 0.0f);
    
    // Pattern playback control (Phase 3)
    void playPattern(const Pattern* pattern);
    void stopPlayback();
    bool isPlaying() const { return transport_.isRolling(); }
    
    // Timeline playback control (Phase 4)
    void playTimeline();
    void playFromTick(Tick startTick);
    
    // Load samples for instruments in project
    bool loadInstrumentSamples();
    
private:
    // Audio render callback
    void audioCallback(jack_nframes_t nframes, float* outL, float* outR);
    
    // Get sample for instrument ID
    std::shared_ptr<Sample> getSampleForInstrument(int instrumentId);
    
    JackAudioBackend audioBackend_;
    Sampler sampler_;
    SampleLibrary sampleLibrary_;
    Transport transport_;
    Scheduler scheduler_;
    Project project_;
    
    // Cache: instrument ID -> loaded sample
    std::unordered_map<int, std::shared_ptr<Sample>> instrumentSamples_;
    
    // Last processed tick (for event scheduling)
    Tick lastProcessedTick_ = 0;
};

} // namespace beater
