#include "domain/Track.hpp"

namespace beater {

Track::Track(const std::string& id, const std::string& name)
    : id_(id), name_(name) {
}

void Track::addRegion(const Region& region) {
    regions_.push_back(region);
    
    // Keep regions sorted by start time for efficient queries
    std::sort(regions_.begin(), regions_.end(),
        [](const Region& a, const Region& b) {
            return a.getStartTick() < b.getStartTick();
        });
}

void Track::removeRegion(const std::string& regionId) {
    regions_.erase(
        std::remove_if(regions_.begin(), regions_.end(),
            [&regionId](const Region& r) { return r.getId() == regionId; }),
        regions_.end()
    );
}

Region* Track::getRegion(const std::string& regionId) {
    auto it = std::find_if(regions_.begin(), regions_.end(),
        [&regionId](const Region& r) { return r.getId() == regionId; });
    
    return (it != regions_.end()) ? &(*it) : nullptr;
}

const Region* Track::getRegion(const std::string& regionId) const {
    auto it = std::find_if(regions_.begin(), regions_.end(),
        [&regionId](const Region& r) { return r.getId() == regionId; });
    
    return (it != regions_.end()) ? &(*it) : nullptr;
}

std::vector<const Region*> Track::getRegionsInRange(Tick startTick, Tick endTick) const {
    std::vector<const Region*> result;
    
    for (const auto& region : regions_) {
        // Region overlaps with query range if:
        // region.start < endTick AND startTick < region.end
        if (region.getStartTick() < endTick && startTick < region.getEndTick()) {
            result.push_back(&region);
        }
    }
    
    return result;
}

bool Track::wouldOverlap(const Region& newRegion) const {
    for (const auto& existing : regions_) {
        if (newRegion.overlaps(existing)) {
            return true;
        }
    }
    return false;
}

void Track::clearRegions() {
    regions_.clear();
}

} // namespace beater
