#include "domain/Track.hpp"
#include "domain/Region.hpp"
#include <iostream>
#include <cassert>

using namespace beater;

void testRegionCreation() {
    Region region("reg_1", RegionType::Groove, 0, 3840);
    
    assert(region.getId() == "reg_1");
    assert(region.getType() == RegionType::Groove);
    assert(region.getStartTick() == 0);
    assert(region.getLengthTicks() == 3840);
    assert(region.getEndTick() == 3840);
    
    std::cout << "✓ testRegionCreation passed\n";
}

void testRegionContains() {
    Region region("reg_1", RegionType::Groove, 1000, 2000);
    
    assert(!region.contains(999));
    assert(region.contains(1000));
    assert(region.contains(2000));
    assert(!region.contains(3000));
    
    std::cout << "✓ testRegionContains passed\n";
}

void testRegionOverlaps() {
    Region r1("r1", RegionType::Groove, 0, 1000);
    Region r2("r2", RegionType::Groove, 500, 1000);   // Overlaps
    Region r3("r3", RegionType::Groove, 1000, 1000);  // Adjacent, no overlap
    Region r4("r4", RegionType::Groove, 2000, 1000);  // No overlap
    
    assert(r1.overlaps(r2));
    assert(r2.overlaps(r1));
    assert(!r1.overlaps(r3));  // Adjacent regions don't overlap
    assert(!r1.overlaps(r4));
    
    std::cout << "✓ testRegionOverlaps passed\n";
}

void testTrackAddRegion() {
    Track track("track_1", "Test Track");
    
    Region r1("r1", RegionType::Groove, 3840, 3840);
    Region r2("r2", RegionType::Groove, 0, 3840);
    
    track.addRegion(r1);
    track.addRegion(r2);
    
    // Regions should be sorted by start tick
    const auto& regions = track.getRegions();
    assert(regions.size() == 2);
    assert(regions[0].getStartTick() == 0);
    assert(regions[1].getStartTick() == 3840);
    
    std::cout << "✓ testTrackAddRegion passed\n";
}

void testTrackGetRegionsInRange() {
    Track track("track_1", "Test Track");
    
    track.addRegion(Region("r1", RegionType::Groove, 0, 1000));
    track.addRegion(Region("r2", RegionType::Groove, 2000, 1000));
    track.addRegion(Region("r3", RegionType::Groove, 4000, 1000));
    
    // Query range [1500, 2500) should only include r2
    auto regions = track.getRegionsInRange(1500, 2500);
    assert(regions.size() == 1);
    assert(regions[0]->getId() == "r2");
    
    // Query range [0, 5000) should include all
    regions = track.getRegionsInRange(0, 5000);
    assert(regions.size() == 3);
    
    // Query range [5000, 6000) should be empty
    regions = track.getRegionsInRange(5000, 6000);
    assert(regions.empty());
    
    std::cout << "✓ testTrackGetRegionsInRange passed\n";
}

void testTrackWouldOverlap() {
    Track track("track_1", "Test Track");
    
    track.addRegion(Region("r1", RegionType::Groove, 1000, 1000));
    track.addRegion(Region("r2", RegionType::Groove, 3000, 1000));
    
    Region newOverlapping("r3", RegionType::Groove, 1500, 1000);
    Region newNonOverlapping("r4", RegionType::Groove, 2000, 500);
    
    assert(track.wouldOverlap(newOverlapping));
    assert(!track.wouldOverlap(newNonOverlapping));
    
    std::cout << "✓ testTrackWouldOverlap passed\n";
}

int main() {
    std::cout << "Running Track tests...\n";
    
    testRegionCreation();
    testRegionContains();
    testRegionOverlaps();
    testTrackAddRegion();
    testTrackGetRegionsInRange();
    testTrackWouldOverlap();
    
    std::cout << "\n✓ All Track tests passed!\n";
    return 0;
}
