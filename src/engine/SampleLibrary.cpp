#include "engine/SampleLibrary.hpp"
#include <sndfile.h>
#include <iostream>
#include <cstring>

namespace beater {

std::shared_ptr<Sample> SampleLibrary::loadSample(const std::string& filepath) {
    // Check cache first
    if (hasSample(filepath)) {
        return cache_[filepath];
    }
    
    SF_INFO sfInfo;
    std::memset(&sfInfo, 0, sizeof(sfInfo));
    
    SNDFILE* file = sf_open(filepath.c_str(), SFM_READ, &sfInfo);
    if (file == nullptr) {
        std::cerr << "Failed to open sample file: " << filepath << "\n";
        std::cerr << "Error: " << sf_strerror(nullptr) << "\n";
        return nullptr;
    }
    
    std::cout << "Loading sample: " << filepath << "\n";
    std::cout << "  Format: " << sfInfo.format << "\n";
    std::cout << "  Channels: " << sfInfo.channels << "\n";
    std::cout << "  Sample rate: " << sfInfo.samplerate << " Hz\n";
    std::cout << "  Frames: " << sfInfo.frames << "\n";
    
    if (sfInfo.channels > 2) {
        std::cerr << "Only mono and stereo samples supported\n";
        sf_close(file);
        return nullptr;
    }
    
    // Create sample object
    auto sample = std::make_shared<Sample>();
    sample->filePath = filepath;
    sample->sampleRate = sfInfo.samplerate;
    sample->channels = sfInfo.channels;
    sample->lengthFrames = sfInfo.frames;
    
    // Read interleaved data
    std::vector<float> interleavedData(sfInfo.frames * sfInfo.channels);
    sf_count_t framesRead = sf_readf_float(file, interleavedData.data(), sfInfo.frames);
    
    if (framesRead != sfInfo.frames) {
        std::cerr << "Warning: Only read " << framesRead << " of " << sfInfo.frames << " frames\n";
        sample->lengthFrames = framesRead;
    }
    
    sf_close(file);
    
    // De-interleave into left/right channels
    sample->dataLeft.resize(sample->lengthFrames);
    sample->dataRight.resize(sample->lengthFrames);
    
    if (sfInfo.channels == 1) {
        // Mono: duplicate to both channels
        for (uint64_t i = 0; i < sample->lengthFrames; ++i) {
            sample->dataLeft[i] = interleavedData[i];
            sample->dataRight[i] = interleavedData[i];
        }
    } else {
        // Stereo: de-interleave
        for (uint64_t i = 0; i < sample->lengthFrames; ++i) {
            sample->dataLeft[i] = interleavedData[i * 2];
            sample->dataRight[i] = interleavedData[i * 2 + 1];
        }
    }
    
    std::cout << "Sample loaded successfully: " << sample->lengthFrames << " frames\n";
    
    // Cache the sample
    cache_[filepath] = sample;
    
    return sample;
}

std::shared_ptr<Sample> SampleLibrary::getSample(const std::string& filepath) {
    auto it = cache_.find(filepath);
    return (it != cache_.end()) ? it->second : nullptr;
}

bool SampleLibrary::hasSample(const std::string& filepath) const {
    return cache_.find(filepath) != cache_.end();
}

void SampleLibrary::unloadSample(const std::string& filepath) {
    cache_.erase(filepath);
}

void SampleLibrary::clear() {
    cache_.clear();
}

} // namespace beater
