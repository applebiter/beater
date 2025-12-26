#include "domain/Pattern.hpp"
#include <iostream>
#include <cassert>

using namespace beater;

void testPatternCreation() {
    Pattern pattern("pat_1", "Basic Groove", 3840);
    
    assert(pattern.getId() == "pat_1");
    assert(pattern.getName() == "Basic Groove");
    assert(pattern.getLengthTicks() == 3840);
    assert(pattern.getNotes().empty());
    
    std::cout << "✓ testPatternCreation passed\n";
}

void testAddNotes() {
    Pattern pattern("pat_1", "Test", 3840);
    
    StepNote kick = {1, 0, 0.9f, 1.0f};
    StepNote snare = {2, 960, 0.8f, 1.0f};
    StepNote hat = {3, 480, 0.6f, 1.0f};
    
    pattern.addNote(kick);
    pattern.addNote(snare);
    pattern.addNote(hat);
    
    assert(pattern.getNotes().size() == 3);
    
    // Notes should be sorted by tick
    const auto& notes = pattern.getNotes();
    assert(notes[0].offsetTick == 0);    // kick
    assert(notes[1].offsetTick == 480);  // hat
    assert(notes[2].offsetTick == 960);  // snare
    
    std::cout << "✓ testAddNotes passed\n";
}

void testGetNotesAt() {
    Pattern pattern("pat_1", "Test", 3840);
    
    pattern.addNote({1, 0, 0.9f, 1.0f});
    pattern.addNote({2, 0, 0.8f, 1.0f});    // Same tick as kick
    pattern.addNote({3, 960, 0.6f, 1.0f});
    
    auto notesAt0 = pattern.getNotesAt(0);
    assert(notesAt0.size() == 2);
    
    auto notesAt960 = pattern.getNotesAt(960);
    assert(notesAt960.size() == 1);
    assert(notesAt960[0].instrumentId == 3);
    
    auto notesAt480 = pattern.getNotesAt(480);
    assert(notesAt480.empty());
    
    std::cout << "✓ testGetNotesAt passed\n";
}

void testPatternLibrary() {
    PatternLibrary library;
    
    Pattern p1("pat_1", "Pattern 1", 3840);
    Pattern p2("pat_2", "Pattern 2", 1920);
    
    library.addPattern(p1);
    library.addPattern(p2);
    
    assert(library.getPatterns().size() == 2);
    assert(library.hasPattern("pat_1"));
    assert(library.hasPattern("pat_2"));
    assert(!library.hasPattern("pat_3"));
    
    const Pattern* retrieved = library.getPattern("pat_1");
    assert(retrieved != nullptr);
    assert(retrieved->getName() == "Pattern 1");
    
    library.removePattern("pat_1");
    assert(library.getPatterns().size() == 1);
    assert(!library.hasPattern("pat_1"));
    
    std::cout << "✓ testPatternLibrary passed\n";
}

int main() {
    std::cout << "Running Pattern tests...\n";
    
    testPatternCreation();
    testAddNotes();
    testGetNotesAt();
    testPatternLibrary();
    
    std::cout << "\n✓ All Pattern tests passed!\n";
    return 0;
}
