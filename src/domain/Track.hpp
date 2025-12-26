#pragma once

#include "domain/Region.hpp"
#include <string>
#include <vector>
#include <algorithm>

namespace beater {

// Track: a horizontal lane containing regions
class Track {
public:
    Track() = default;
    Track(const std::string& id, const std::string& name);
    
    // Accessors
    const std::string& getId() const { return id_; }
    const std::string& getName() const { return name_; }
    bool isMuted() const { return muted_; }
    bool isSoloed() const { return soloed_; }
    const std::vector<Region>& getRegions() const { return regions_; }
    
    // Mutators
    void setName(const std::string& name) { name_ = name; }
    void setMuted(bool muted) { muted_ = muted; }
    void setSoloed(bool soloed) { soloed_ = soloed; }
    
    // Region management
    void addRegion(const Region& region);
    void removeRegion(const std::string& regionId);
    Region* getRegion(const std::string& regionId);
    const Region* getRegion(const std::string& regionId) const;
    
    // Find regions in a time range
    std::vector<const Region*> getRegionsInRange(Tick startTick, Tick endTick) const;
    
    // Check if adding this region would create an overlap (for MVP)
    bool wouldOverlap(const Region& newRegion) const;
    
    // Clear all regions
    void clearRegions();
    
private:
    std::string id_;
    std::string name_ = "Track";
    bool muted_ = false;
    bool soloed_ = false;
    std::vector<Region> regions_;
};

} // namespace beater
