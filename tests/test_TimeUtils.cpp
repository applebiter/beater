#include "domain/TimeTypes.hpp"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace beater;

void testTicksPerBar() {
    TimeSignature sig_4_4 = {4, 4};
    TimeSignature sig_3_4 = {3, 4};
    TimeSignature sig_6_8 = {6, 8};
    
    assert(TimeUtils::ticksPerBar(sig_4_4) == 3840);  // 4 * 960
    assert(TimeUtils::ticksPerBar(sig_3_4) == 2880);  // 3 * 960
    assert(TimeUtils::ticksPerBar(sig_6_8) == 2880);  // (6/8) * 4 * 960 = 2880
    
    std::cout << "✓ testTicksPerBar passed\n";
}

void testTicksPerBeat() {
    TimeSignature sig_4_4 = {4, 4};
    TimeSignature sig_6_8 = {6, 8};
    
    assert(TimeUtils::ticksPerBeat(sig_4_4) == 960);   // Quarter note
    assert(TimeUtils::ticksPerBeat(sig_6_8) == 480);   // Eighth note
    
    std::cout << "✓ testTicksPerBeat passed\n";
}

void testSnapToBar() {
    TimeSignature sig_4_4 = {4, 4};
    
    // Exact bar boundaries
    assert(TimeUtils::snapToBar(0, sig_4_4) == 0);
    assert(TimeUtils::snapToBar(3840, sig_4_4) == 3840);
    
    // Snap closer to start of bar
    assert(TimeUtils::snapToBar(1000, sig_4_4) == 0);
    
    // Snap closer to next bar
    assert(TimeUtils::snapToBar(3000, sig_4_4) == 3840);
    
    std::cout << "✓ testSnapToBar passed\n";
}

void testSnapToBeat() {
    TimeSignature sig_4_4 = {4, 4};
    
    // Exact beat boundaries
    assert(TimeUtils::snapToBeat(0, sig_4_4) == 0);
    assert(TimeUtils::snapToBeat(960, sig_4_4) == 960);
    
    // Snap to nearest beat
    assert(TimeUtils::snapToBeat(400, sig_4_4) == 0);
    assert(TimeUtils::snapToBeat(600, sig_4_4) == 960);
    
    std::cout << "✓ testSnapToBeat passed\n";
}

void testSnapToGrid() {
    // Quarter note grid
    assert(TimeUtils::snapToGrid(0, 1) == 0);
    assert(TimeUtils::snapToGrid(960, 1) == 960);
    assert(TimeUtils::snapToGrid(400, 1) == 0);
    assert(TimeUtils::snapToGrid(600, 1) == 960);
    
    // Eighth note grid
    assert(TimeUtils::snapToGrid(240, 2) == 480);  // Snap to nearest eighth (480 ticks)
    
    // Sixteenth note grid
    assert(TimeUtils::snapToGrid(100, 4) == 0);    // Snap to start
    
    std::cout << "✓ testSnapToGrid passed\n";
}

void testTickFrameConversion() {
    double bpm = 120.0;
    uint32_t sampleRate = 48000;
    
    // At 120 BPM, one quarter note = 0.5 seconds = 24000 frames @ 48kHz
    // One quarter = 960 ticks
    // So 960 ticks = 24000 frames
    // framesPerTick = 24000 / 960 = 25
    
    double fpt = TimeUtils::framesPerTick(bpm, sampleRate);
    assert(std::abs(fpt - 25.0) < 0.01);
    
    // One bar (3840 ticks) should be 96000 frames
    uint64_t frames = TimeUtils::ticksToFrames(3840, bpm, sampleRate);
    assert(frames == 96000);
    
    // Convert back
    Tick ticks = TimeUtils::framesToTicks(96000, bpm, sampleRate);
    assert(ticks == 3840);
    
    std::cout << "✓ testTickFrameConversion passed\n";
}

void testTickToPosition() {
    TimeSignature sig_4_4 = {4, 4};
    
    // Start of song
    MusicalPosition pos = TimeUtils::tickToPosition(0, sig_4_4);
    assert(pos.bar == 0 && pos.beat == 0 && pos.tick == 0);
    
    // One beat in
    pos = TimeUtils::tickToPosition(960, sig_4_4);
    assert(pos.bar == 0 && pos.beat == 1 && pos.tick == 0);
    
    // Start of bar 2
    pos = TimeUtils::tickToPosition(3840, sig_4_4);
    assert(pos.bar == 1 && pos.beat == 0 && pos.tick == 0);
    
    // Somewhere in bar 2, beat 3, with offset
    pos = TimeUtils::tickToPosition(3840 + 1920 + 100, sig_4_4);
    assert(pos.bar == 1 && pos.beat == 2 && pos.tick == 100);
    
    std::cout << "✓ testTickToPosition passed\n";
}

void testPositionToTick() {
    TimeSignature sig_4_4 = {4, 4};
    
    MusicalPosition pos;
    
    // Bar 0, beat 0
    pos = {0, 0, 0};
    assert(TimeUtils::positionToTick(pos, sig_4_4) == 0);
    
    // Bar 0, beat 1
    pos = {0, 1, 0};
    assert(TimeUtils::positionToTick(pos, sig_4_4) == 960);
    
    // Bar 1, beat 0
    pos = {1, 0, 0};
    assert(TimeUtils::positionToTick(pos, sig_4_4) == 3840);
    
    // Bar 1, beat 2, tick 100
    pos = {1, 2, 100};
    assert(TimeUtils::positionToTick(pos, sig_4_4) == 3840 + 1920 + 100);
    
    std::cout << "✓ testPositionToTick passed\n";
}

int main() {
    std::cout << "Running TimeUtils tests...\n";
    
    testTicksPerBar();
    testTicksPerBeat();
    testSnapToBar();
    testSnapToBeat();
    testSnapToGrid();
    testTickFrameConversion();
    testTickToPosition();
    testPositionToTick();
    
    std::cout << "\n✓ All TimeUtils tests passed!\n";
    return 0;
}
