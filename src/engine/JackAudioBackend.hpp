#pragma once

#include <jack/jack.h>
#include <string>
#include <functional>
#include <atomic>

namespace beater {

// Callback function type for audio rendering
// Parameters: nframes, outL, outR
using AudioCallback = std::function<void(jack_nframes_t, float*, float*)>;

// JACK audio backend for real-time audio output
class JackAudioBackend {
public:
    JackAudioBackend();
    ~JackAudioBackend();
    
    // Initialize JACK client and create ports
    bool initialize(const std::string& clientName);
    
    // Shutdown and cleanup
    void shutdown();
    
    // Check if JACK is running and connected
    bool isActive() const { return client_ != nullptr; }
    
    // Set the audio rendering callback
    void setAudioCallback(AudioCallback callback) { audioCallback_ = callback; }
    
    // Get current sample rate
    uint32_t getSampleRate() const { return sampleRate_; }
    
    // Get current buffer size
    jack_nframes_t getBufferSize() const { return bufferSize_; }
    
    // Get transport state
    jack_position_t getTransportPosition() const;
    bool isTransportRolling() const;
    
private:
    // JACK callbacks
    static int processCallback(jack_nframes_t nframes, void* arg);
    static int sampleRateCallback(jack_nframes_t nframes, void* arg);
    static int bufferSizeCallback(jack_nframes_t nframes, void* arg);
    static void shutdownCallback(void* arg);
    static int xrunCallback(void* arg);
    
    jack_client_t* client_ = nullptr;
    jack_port_t* outPortLeft_ = nullptr;
    jack_port_t* outPortRight_ = nullptr;
    
    AudioCallback audioCallback_;
    
    std::atomic<uint32_t> sampleRate_{48000};
    std::atomic<jack_nframes_t> bufferSize_{256};
    std::atomic<uint32_t> xrunCount_{0};
};

} // namespace beater
