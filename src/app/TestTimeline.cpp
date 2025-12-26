#include "engine/Engine.hpp"
#include "domain/Pattern.hpp"
#include "domain/Region.hpp"
#include "domain/Track.hpp"
#include "domain/Instrument.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace beater;

int main() {
    std::cout << "=== Beater Timeline Test ===\n\n";
    
    // Create engine
    Engine engine;
    
    if (!engine.initialize("beater_timeline")) {
        std::cerr << "Failed to initialize engine\n";
        return 1;
    }
    
    std::cout << "Engine initialized:\n";
    std::cout << "  Sample rate: " << engine.getSampleRate() << " Hz\n";
    std::cout << "  Buffer size: " << engine.getBufferSize() << " frames\n\n";
    
    // Get project reference
    Project& project = engine.getProject();
    
    // Create drum instruments
    Instrument kick(0, "Kick");
    kick.setSamplePath("/usr/share/hydrogen/data/drumkits/GMRockKit/Kick-Hard.wav");
    
    Instrument snare(1, "Snare");
    snare.setSamplePath("/usr/share/hydrogen/data/drumkits/GMRockKit/Snare-Hard.wav");
    
    Instrument hat(2, "Hi-Hat");
    hat.setSamplePath("/usr/share/hydrogen/data/drumkits/GMRockKit/HatClosed-Hard.wav");
    
    Instrument crash(3, "Crash");
    crash.setSamplePath("/usr/share/hydrogen/data/drumkits/GMRockKit/Crash-Hard.wav");
    
    project.getInstrumentRack().addInstrument(kick);
    project.getInstrumentRack().addInstrument(snare);
    project.getInstrumentRack().addInstrument(hat);
    project.getInstrumentRack().addInstrument(crash);
    
    // Load samples
    std::cout << "Loading samples...\n";
    if (!engine.loadInstrumentSamples()) {
        std::cerr << "Failed to load instrument samples\n";
        return 1;
    }
    std::cout << "\n";
    
    // Create Pattern 1: Basic 4/4 groove
    Tick barLength = TimeUtils::ticksPerBar({4, 4});
    Pattern groove("groove1", "Basic Groove", barLength);
    
    // Kick on 1 and 3
    groove.addNote({0, 0, 0.9f});
    groove.addNote({0, 1920, 0.85f});
    
    // Snare on 2 and 4
    groove.addNote({1, 960, 0.8f});
    groove.addNote({1, 2880, 0.8f});
    
    // Hi-hat 8th notes
    for (int i = 0; i < 8; ++i) {
        groove.addNote({2, i * 480, 0.6f});
    }
    
    // Create Pattern 2: Fill pattern
    Pattern fill("fill1", "Drum Fill", barLength);
    
    // Rapid snare hits
    for (int i = 0; i < 16; ++i) {
        float vel = 0.6f + (i * 0.02f);  // Crescendo
        fill.addNote({1, i * 240, vel});
    }
    
    // Crash at the end
    fill.addNote({3, barLength - 10, 0.9f});
    
    // Create Pattern 3: Half-time feel
    Pattern halftime("halftime1", "Half-Time", barLength);
    
    // Kick on 1 only
    halftime.addNote({0, 0, 0.9f});
    
    // Snare on 3
    halftime.addNote({1, 1920, 0.85f});
    
    // Hi-hat quarter notes
    for (int i = 0; i < 4; ++i) {
        halftime.addNote({2, i * 960, 0.65f});
    }
    
    // Add patterns to library
    project.getPatternLibrary().addPattern(groove);
    project.getPatternLibrary().addPattern(fill);
    project.getPatternLibrary().addPattern(halftime);
    
    // Create timeline arrangement
    // Add a drum track if none exists, or use the first track
    if (project.getTrackCount() == 0) {
        Track drumTrack("track1", "Drums");
        project.addTrack(drumTrack);
    }
    Track* drumTrack = project.getTrack(0);
    
    // Bars 1-4: Basic groove (4 bars)
    Region grooveRegion1("region1", RegionType::Groove, 0, 4 * barLength);
    grooveRegion1.setPatternId("groove1");
    drumTrack->addRegion(grooveRegion1);
    
    // Bar 5: Fill
    Region fillRegion("region2", RegionType::Fill, 4 * barLength, barLength);
    fillRegion.setPatternId("fill1");
    drumTrack->addRegion(fillRegion);
    
    // Bars 6-9: Half-time (4 bars)
    Region halftimeRegion("region3", RegionType::Groove, 5 * barLength, 4 * barLength);
    halftimeRegion.setPatternId("halftime1");
    drumTrack->addRegion(halftimeRegion);
    
    // Bar 10: Fill again
    Region fillRegion2("region4", RegionType::Fill, 9 * barLength, barLength);
    fillRegion2.setPatternId("fill1");
    drumTrack->addRegion(fillRegion2);
    
    // Bars 11-14: Back to groove (4 bars)
    Region grooveRegion2("region5", RegionType::Groove, 10 * barLength, 4 * barLength);
    grooveRegion2.setPatternId("groove1");
    drumTrack->addRegion(grooveRegion2);
    
    std::cout << "Created timeline arrangement:\n";
    std::cout << "  Bars 1-4:  Basic Groove (4/4)\n";
    std::cout << "  Bar 5:     Fill\n";
    std::cout << "  Bars 6-9:  Half-Time Feel\n";
    std::cout << "  Bar 10:    Fill\n";
    std::cout << "  Bars 11-14: Basic Groove\n";
    std::cout << "  Total: 14 bars = " << (14 * barLength) << " ticks\n";
    std::cout << "\n";
    
    // Play the timeline
    std::cout << "Playing timeline arrangement (120 BPM)...\n";
    std::cout << "Press Ctrl+C to stop.\n\n";
    
    engine.playTimeline();
    
    // Let it play for about 28 seconds (14 bars at 120 BPM = ~28 seconds)
    for (int i = 0; i < 28; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
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
