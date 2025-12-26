# Beater - Linux Drum Machine

A JACK-aware drum machine with timeline-based beat block arrangement for Linux.

## Status: Phase 1 Complete ✓

**Domain Model & Foundation** - Fully implemented and tested.

### Completed Features
- ✓ Project structure and CMake build system
- ✓ Time types (Tick, TimeSignature, musical time math)
- ✓ Pattern model (Hydrogen-like step/note representation)
- ✓ Region & Track classes (timeline arrangement)
- ✓ Tempo and Meter maps
- ✓ Instrument rack
- ✓ Project document structure
- ✓ Unit tests for core time utilities

### Next Steps (Phase 2)
- JACK integration
- Sample loading (libsndfile)
- Basic sampler engine
- Manual sample trigger testing

## Building

### Dependencies
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake \
    libjack-jackd2-dev libsndfile1-dev \
    qt6-base-dev

# Arch Linux
sudo pacman -S base-devel cmake jack2 libsndfile qt6-base
```

### Compile
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Run Tests
```bash
cd build
ctest --verbose
```

### Run Application
```bash
./src/beater
```

## Architecture

```
beater/
├── src/
│   ├── domain/      # Core data structures (no dependencies)
│   │   ├── TimeTypes     # Musical time, tempo, meter
│   │   ├── Pattern       # Note sequences
│   │   ├── Region        # Timeline blocks
│   │   ├── Track         # Arrangement lanes
│   │   ├── Instrument    # Sample mapping
│   │   └── Project       # Top-level document
│   ├── engine/      # Audio processing (JACK, sampler)
│   ├── ui/          # Qt6 interface
│   ├── serialization/ # JSON save/load
│   └── app/         # Main entry point
└── tests/          # Unit tests
```

## Design Principles

1. **Clear ownership**: UI owns document, engine owns compiled playback model
2. **RT-safety first**: No allocations/locks in JACK audio callback
3. **Musical time primary**: Edit in ticks, convert to frames only for scheduling
4. **Snap-to-bar (MVP)**: Regions quantize to bar boundaries initially

## License

To be determined.

## References

- Project plan: [PROJECT_PLAN.md](PROJECT_PLAN.md)
- JACK Audio: https://jackaudio.org/
- Hydrogen Drum Machine: http://hydrogen-music.org/
