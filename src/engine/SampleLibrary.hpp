#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace beater {

// Audio sample data
struct Sample {
    std::vector<float> dataLeft;
    std::vector<float> dataRight;
    uint32_t sampleRate = 48000;
    uint32_t channels = 2;
    uint64_t lengthFrames = 0;
    std::string filePath;
    
    bool isMono() const { return channels == 1; }
    bool isStereo() const { return channels == 2; }
};

// Sample library: loads and caches audio samples
class SampleLibrary {
public:
    SampleLibrary() = default;
    ~SampleLibrary() = default;
    
    // Load a sample from file
    // Returns nullptr on failure
    std::shared_ptr<Sample> loadSample(const std::string& filepath);
    
    // Get a cached sample (returns nullptr if not loaded)
    std::shared_ptr<Sample> getSample(const std::string& filepath);
    
    // Check if sample is already loaded
    bool hasSample(const std::string& filepath) const;
    
    // Unload a specific sample
    void unloadSample(const std::string& filepath);
    
    // Clear all loaded samples
    void clear();
    
    // Get cache size
    size_t getCacheSize() const { return cache_.size(); }
    
private:
    std::unordered_map<std::string, std::shared_ptr<Sample>> cache_;
};

} // namespace beater
