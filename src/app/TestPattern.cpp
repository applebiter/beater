#include "engine/Engine.hpp"
#include "domain/Pattern.hpp"
#include "domain/Instrument.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace beater;

int main() {
    std::cout << "=== Beater Pattern Test ===\n\n";
    
    // Create engine
    Engine engine;
    
    if (!engine.initialize("beater_pattern")) {
        std::cerr << "Failed to initialize engine\n";
        return 1;
    }
    
    std::cout << "Engine initialized:\n";
    std::cout << "  Sample rate: " << engine.getSampleRate() << " Hz\n";
    std::cout << "  Buffer size: " << engine.getBufferSize() << " frames\n\n";
    
    // Create a simple project with instruments
    Project& project = engine.getProject();
    
    // Create three drum instruments: Kick, Snare, Hat
    Instrument kick(0, "Kick");
    kick.setSamplePath("/usr/share/hydrogen/data/drumkits/GMRockKit/Kick-Hard.wav");
    
    Instrument snare(1, "Snare");
    snare.setSamplePath("/usr/share/hydrogen/data/drumkits/GMRockKit/Snare-Hard.wav");
    
    Instrument hat(2, "Hat");
    hat.setSamplePath("/usr/share/hydrogen/data/drumkits/GMRockKit/HatClosed-Hard.wav");
    
    project.getInstrumentRack().addInstrument(kick);
    project.getInstrumentRack().addInstrument(snare);
    project.getInstrumentRack().addInstrument(hat);
    
    // Load samples for all instruments
    std::cout << "Loading samples...\n";
    if (!engine.loadInstrumentSamples()) {
        std::cerr << "Failed to load instrument samples\n";
        return 1;
    }
    std::cout << "\n";
    
    // Create a simple pattern: 4/4 beat with kick, snare, and hi-hat
    Pattern pattern("pat1", "Basic Beat", TimeUtils::ticksPerBar({4, 4}));
    
    // Kick on beats 1 and 3 (ticks 0, 1920)
    pattern.addNote({0, 0, 0.9f});         // {instrumentId, offsetTick, velocity}
    pattern.addNote({0, 1920, 0.85f});     // Beat 3
    
    // Snare on beats 2 and 4 (ticks 960, 2880)
    pattern.addNote({1, 960, 0.8f});       // Beat 2
    pattern.addNote({1, 2880, 0.8f});      // Beat 4
    
    // Hi-hat on 8th notes (every 480 ticks)
    for (int i = 0; i < 8; ++i) {
        pattern.addNote({2, i * 480, 0.6f});
    }
    
    std::cout << "Created pattern: " << pattern.getName() << "\n";
    std::cout << "  Length: " << pattern.getLengthTicks() << " ticks (1 bar)\n";
    std::cout << "  Notes: " << pattern.getNotes().size() << "\n";
    std::cout << "  Pattern layout:\n";
    std::cout << "    Kick:  |X...|..X.|....|....|  (beats 1, 3)\n";
    std::cout << "    Snare: |....|X...|....|X...|  (beats 2, 4)\n";
    std::cout << "    Hat:   |X.X.|X.X.|X.X.|X.X.|  (8th notes)\n";
    std::cout << "\n";
    
    // Start playing the pattern
    std::cout << "Playing pattern (120 BPM, looping)...\n";
    std::cout << "Press Ctrl+C to stop.\n\n";
    
    engine.playPattern(&pattern);
    
    // Run for a while
    for (int i = 0; i < 8; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        if (engine.isPlaying()) {
            std::cout << "." << std::flush;
        }
    }
    
    std::cout << "\n\nStopping playback...\n";
    engine.stopPlayback();
    
    // Wait a bit for voices to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    engine.shutdown();
    std::cout << "Done!\n";
    
    return 0;
}
