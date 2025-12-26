# Beater - Drum Machine

A JACK-aware drum machine with timeline-based pattern arrangement for Linux.

![Version](https://img.shields.io/badge/version-0.1.0-blue)
![Platform](https://img.shields.io/badge/platform-Linux-lightgrey)
![License](https://img.shields.io/badge/license-TBD-orange)

## Status: Phase 6 Complete ✓

**Interactive UI with Project Persistence** - Fully functional drum machine!

### Current Features
- 🎵 **JACK Audio Integration** - Real-time low-latency drum playback
- 🎨 **Interactive Timeline Editor** - Drag, drop, resize regions with snap-to-grid
- 📊 **Pattern Library** - Groove, fill, and half-time patterns
- 🎼 **Dynamic Time Signatures** - Right-click regions to change time signature
- ⏱️ **Tempo Control** - 40-300 BPM with real-time adjustment
- 🔍 **Zoom Controls** - Mouse wheel zoom (Ctrl+scroll) and toolbar buttons
- ⌨️ **Keyboard Shortcuts** - Delete key to remove regions, standard shortcuts
- 💾 **Project Persistence** - Save/load projects in JSON format (.beater files)
- 🎚️ **Transport Controls** - Play, stop, rewind with visual playhead
- 🌍 **Cross-Platform Sample Detection** - Automatically finds samples in common locations
- 🎨 **Dark Theme UI** - Professional modern interface
- 📍 **Playhead Control** - Click to position, drag to scrub

### Next Phase
- Pattern editor UI with step sequencer view

## Installation

### Quick Start (Binary)
Download the latest release from [Releases](https://github.com/applebiter/beater/releases):

```bash
# Download and extract
wget https://github.com/applebiter/beater/releases/download/v0.1.0/beater
chmod +x beater

# Install dependencies
sudo apt install jackd2 qjackctl libqt6widgets6 libsndfile1

# Start JACK (use qjackctl or command line)
qjackctl &

# Run Beater
./beater
```

### Building from Source

#### Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake git \
    libjack-jackd2-dev libsndfile1-dev \
    qt6-base-dev libqt6widgets6

# Arch Linux
sudo pacman -S base-devel cmake git jack2 libsndfile qt6-base
```

#### Compile
```bash
git clone https://github.com/applebiter/beater.git
cd beater
mkdir build
cd build
cmake ..
make -j$(nproc)
```

#### Run
```bash
# Start JACK first
qjackctl &

# Run from build directory
./src/beater
```

## Usage

### Getting Started
1. **Start JACK** - Use qjackctl or start JACK manually
2. **Launch Beater** - The application will auto-detect drum samples
3. **Drag Patterns** - Drag pattern blocks from the palette to the timeline
4. **Edit Timeline** - Click, drag, resize, and delete regions
5. **Change Time Signatures** - Right-click regions to set custom time signatures
6. **Adjust Tempo** - Use the tempo spinbox or type a value
7. **Play/Stop** - Use transport controls or spacebar
8. **Save Projects** - File → Save (Ctrl+S)

### Keyboard Shortcuts
- `Ctrl+N` - New project
- `Ctrl+O` - Open project
- `Ctrl+S` - Save project
- `Ctrl+Shift+S` - Save As
- `Ctrl+,` - Settings
- `Delete` or `Backspace` - Delete selected region
- `Ctrl+Scroll` - Zoom timeline

### Sample Configuration
Beater automatically searches for drum samples in:
- `/usr/share/hydrogen/data/drumkits/GMRockKit/`
- `/usr/local/share/hydrogen/data/drumkits/GMRockKit/`
- `~/.hydrogen/data/drumkits/GMRockKit/`
- `./samples/` (relative to executable)

To add custom sample directories: **File → Settings**

## Project File Format

Projects are saved as JSON files with `.beater` extension:
```json
{
  "version": 1,
  "name": "My Project",
  "patterns": [...],
  "tracks": [...],
  "instruments": [...],
  "meterChanges": [...]
}
```

## Architecture

```
beater/
├── src/
│   ├── domain/          # Core data structures
│   │   ├── TimeTypes    # Musical time, tempo, meter
│   │   ├── Pattern      # Note sequences
│   │   ├── Region       # Timeline blocks
│   │   ├── Track        # Arrangement lanes
│   │   ├── Instrument   # Sample mapping
│   │   └── Project      # Top-level document
│   ├── engine/          # Audio processing
│   │   ├── JackAudioBackend  # JACK integration
│   │   ├── Sampler      # Sample playback
│   │   ├── Scheduler    # Event scheduling
│   │   └── Transport    # Playback control
│   ├── ui/              # Qt6 interface
│   │   ├── MainWindow   # Main application window
│   │   ├── TimelineWidget    # Timeline editor
│   │   └── PatternPalette    # Pattern library
│   ├── serialization/   # JSON save/load
│   └── app/             # Main entry point
├── tests/               # Unit tests
└── external/            # Third-party dependencies (nlohmann/json)
```

## Technical Details

- **Language**: C++17
- **UI Framework**: Qt6 (6.4.2+)
- **Audio Backend**: JACK Audio Connection Kit
- **Build System**: CMake 3.16+
- **Sample Loading**: libsndfile
- **Serialization**: nlohmann/json
- **Time Resolution**: 960 PPQ (Pulses Per Quarter note)

## Troubleshooting

### "Failed to load samples"
- Install Hydrogen drum machine: `sudo apt install hydrogen`
- Or configure custom sample directories in Settings

### JACK Connection Issues
- Ensure JACK is running: `jack_lsp` should list ports
- Check JACK settings in qjackctl (try 48000 Hz, 256 frames)
- Verify Beater appears in JACK connections

### Qt6 Not Found
- Ubuntu 22.04+: `sudo apt install qt6-base-dev`
- Older Ubuntu: Add Qt6 PPA or build from source

## Contributing

Contributions welcome! Areas for improvement:
- Windows/Mac audio backend support
- Additional pattern types
- MIDI I/O
- LV2 plugin hosting
- Improved sample management

## License

To be determined.

## Credits

- Inspired by Hydrogen Drum Machine
- Uses JACK Audio Connection Kit
- Qt6 framework for UI

## Links

- **GitHub**: https://github.com/applebiter/beater
- **Issues**: https://github.com/applebiter/beater/issues
- **Releases**: https://github.com/applebiter/beater/releases
- **Project Plan**: [PROJECT_PLAN.md](PROJECT_PLAN.md)

---

**Note**: This is an early release focused on Linux with JACK. Windows/Mac support and additional features are planned for future releases.
