#include "engine/Engine.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace beater;

int main(int argc, char* argv[]) {
    std::cout << "=== Beater Audio Engine Test ===\n\n";
    
    // Create engine
    Engine engine;
    
    // Initialize JACK
    std::cout << "Initializing JACK...\n";
    if (!engine.initialize("beater_test")) {
        std::cerr << "Failed to initialize JACK. Is jackd running?\n";
        std::cerr << "Try: jackd -d alsa (or: jackd -d dummy for testing without hardware)\n";
        return 1;
    }
    
    std::cout << "\nJACK initialized successfully!\n";
    std::cout << "Sample rate: " << engine.getSampleRate() << " Hz\n";
    std::cout << "Buffer size: " << engine.getBufferSize() << " frames\n";
    
    // Load a sample if provided
    if (argc > 1) {
        std::string samplePath = argv[1];
        std::cout << "\nLoading sample: " << samplePath << "\n";
        
        auto sample = engine.getSampleLibrary().loadSample(samplePath);
        if (sample) {
            std::cout << "Sample loaded successfully!\n";
            std::cout << "Playing sample in 1 second...\n";
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            // Trigger the sample
            std::cout << "Triggering sample...\n";
            engine.triggerSample(sample, 0.8f, 1.0f, 0.0f);
            
            // Wait for playback
            float durationSec = static_cast<float>(sample->lengthFrames) / sample->sampleRate;
            std::cout << "Sample duration: " << durationSec << " seconds\n";
            std::cout << "Playing...\n";
            
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(durationSec * 1000) + 500));
            
            std::cout << "Playback complete!\n";
        } else {
            std::cerr << "Failed to load sample\n";
        }
    } else {
        std::cout << "\nNo sample provided. Keeping JACK active for 5 seconds...\n";
        std::cout << "(Connect ports with qjackctl or jack_connect)\n";
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    
    std::cout << "\nShutting down...\n";
    engine.shutdown();
    
    std::cout << "Test complete!\n";
    return 0;
}
