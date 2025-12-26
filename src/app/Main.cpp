#include "ui/MainWindow.hpp"
#include "engine/Engine.hpp"
#include "domain/Pattern.hpp"
#include "domain/Region.hpp"
#include "domain/Track.hpp"
#include "domain/Instrument.hpp"
#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <iostream>
#include <memory>
#include <vector>

using namespace beater;

// Helper function to find sample in common locations
std::string findSample(const std::string& sampleName) {
    std::vector<std::string> searchPaths = {
        // Linux common locations
        "/usr/share/hydrogen/data/drumkits/GMRockKit/",
        "/usr/local/share/hydrogen/data/drumkits/GMRockKit/",
        "~/.hydrogen/data/drumkits/GMRockKit/",
        // Relative to executable (cross-platform)
        "./samples/",
        "../samples/",
        "../../samples/",
        // Windows common locations (if we port)
        "C:/Program Files/Hydrogen/data/drumkits/GMRockKit/",
        "C:/Program Files (x86)/Hydrogen/data/drumkits/GMRockKit/"
    };
    
    for (const auto& path : searchPaths) {
        QString fullPath = QString::fromStdString(path + sampleName);
        
        // Expand ~ to home directory
        if (fullPath.startsWith("~/")) {
            fullPath = QDir::homePath() + fullPath.mid(1);
        }
        
        if (QFileInfo::exists(fullPath)) {
            return fullPath.toStdString();
        }
    }
    
    return ""; // Not found
}

int main(int argc, char* argv[]) {
    std::cout << "=== Beater Drum Machine v0.1.0 ===\n";
    std::cout << "Phase 5: Basic UI with Transport Controls\n\n";
    
    // Initialize Qt application
    QApplication app(argc, argv);
    
    // Create engine
    std::unique_ptr<Engine> engine = std::make_unique<Engine>();
    
    if (!engine->initialize("beater")) {
        std::cerr << "Failed to initialize audio engine\n";
        return 1;
    }
    
    std::cout << "Engine initialized:\n";
    std::cout << "  Sample rate: " << engine->getSampleRate() << " Hz\n";
    std::cout << "  Buffer size: " << engine->getBufferSize() << " frames\n\n";
    
    // Set up a demo project with timeline
    Project& project = engine->getProject();
    
    // Create instruments with auto-detected sample paths
    std::cout << "Searching for drum samples...\n";
    
    Instrument kick(0, "Kick");
    std::string kickPath = findSample("Kick-Hard.wav");
    if (!kickPath.empty()) {
        kick.setSamplePath(kickPath);
        std::cout << "  Found Kick: " << kickPath << "\n";
    } else {
        std::cout << "  Warning: Kick sample not found\n";
    }
    
    Instrument snare(1, "Snare");
    std::string snarePath = findSample("Snare-Hard.wav");
    if (!snarePath.empty()) {
        snare.setSamplePath(snarePath);
        std::cout << "  Found Snare: " << snarePath << "\n";
    } else {
        std::cout << "  Warning: Snare sample not found\n";
    }
    
    Instrument hat(2, "Hi-Hat");
    std::string hatPath = findSample("HatClosed-Hard.wav");
    if (!hatPath.empty()) {
        hat.setSamplePath(hatPath);
        std::cout << "  Found Hi-Hat: " << hatPath << "\n";
    } else {
        std::cout << "  Warning: Hi-Hat sample not found\n";
    }
    
    Instrument crash(3, "Crash");
    std::string crashPath = findSample("Crash-Hard.wav");
    if (!crashPath.empty()) {
        crash.setSamplePath(crashPath);
        std::cout << "  Found Crash: " << crashPath << "\n";
    } else {
        std::cout << "  Warning: Crash sample not found\n";
    }
    
    project.getInstrumentRack().addInstrument(kick);
    project.getInstrumentRack().addInstrument(snare);
    project.getInstrumentRack().addInstrument(hat);
    project.getInstrumentRack().addInstrument(crash);
    
    // Load samples (gracefully handle missing samples)
    std::cout << "\nLoading drum samples...\n";
    if (!engine->loadInstrumentSamples()) {
        std::cout << "Note: Some or all samples could not be loaded.\n";
        std::cout << "The application will still run, but playback may be silent.\n";
        std::cout << "Use File > Settings to configure sample directories.\n\n";
    } else {
        std::cout << "Samples loaded successfully!\n\n";
    }
    
    // Create patterns
    Tick barLength = TimeUtils::ticksPerBar({4, 4});
    
    Pattern groove("groove1", "Basic Groove", barLength);
    groove.addNote({0, 0, 0.9f});
    groove.addNote({0, 1920, 0.85f});
    groove.addNote({1, 960, 0.8f});
    groove.addNote({1, 2880, 0.8f});
    for (int i = 0; i < 8; ++i) {
        groove.addNote({2, i * 480, 0.6f});
    }
    
    Pattern fill("fill1", "Drum Fill", barLength);
    for (int i = 0; i < 16; ++i) {
        fill.addNote({1, i * 240, 0.6f + (i * 0.02f)});
    }
    fill.addNote({3, barLength - 10, 0.9f});
    
    Pattern halftime("halftime1", "Half-Time", barLength);
    halftime.addNote({0, 0, 0.9f});
    halftime.addNote({1, 1920, 0.85f});
    for (int i = 0; i < 4; ++i) {
        halftime.addNote({2, i * 960, 0.65f});
    }
    
    project.getPatternLibrary().addPattern(groove);
    project.getPatternLibrary().addPattern(fill);
    project.getPatternLibrary().addPattern(halftime);
    
    // Create timeline arrangement
    if (project.getTrackCount() == 0) {
        Track drumTrack("track1", "Drums");
        project.addTrack(drumTrack);
    }
    Track* drumTrack = project.getTrack(0);
    
    Region grooveRegion1("region1", RegionType::Groove, 0, 4 * barLength);
    grooveRegion1.setPatternId("groove1");
    drumTrack->addRegion(grooveRegion1);
    
    Region fillRegion("region2", RegionType::Fill, 4 * barLength, barLength);
    fillRegion.setPatternId("fill1");
    drumTrack->addRegion(fillRegion);
    
    Region halftimeRegion("region3", RegionType::Groove, 5 * barLength, 4 * barLength);
    halftimeRegion.setPatternId("halftime1");
    drumTrack->addRegion(halftimeRegion);
    
    Region fillRegion2("region4", RegionType::Fill, 9 * barLength, barLength);
    fillRegion2.setPatternId("fill1");
    drumTrack->addRegion(fillRegion2);
    
    Region grooveRegion2("region5", RegionType::Groove, 10 * barLength, 4 * barLength);
    grooveRegion2.setPatternId("groove1");
    drumTrack->addRegion(grooveRegion2);
    
    std::cout << "Timeline created: 14 bars with multiple patterns\n\n";
    
    // Create and show main window
    MainWindow window;
    window.setEngine(engine.get());
    window.setProject(&project);
    window.show();
    
    std::cout << "UI ready. Use Play/Stop buttons to control playback.\n";
    
    int result = app.exec();
    
    // Clean shutdown
    engine->shutdown();
    
    return result;
}

