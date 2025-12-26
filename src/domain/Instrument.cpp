#include "domain/Instrument.hpp"
#include <algorithm>

namespace beater {

Instrument::Instrument(int id, const std::string& name)
    : id_(id), name_(name) {
}

// InstrumentRack implementation

void InstrumentRack::addInstrument(const Instrument& instrument) {
    // Remove existing instrument with same ID if present
    removeInstrument(instrument.getId());
    instruments_.push_back(instrument);
    
    // Keep sorted by ID for consistent ordering
    std::sort(instruments_.begin(), instruments_.end(),
        [](const Instrument& a, const Instrument& b) {
            return a.getId() < b.getId();
        });
}

void InstrumentRack::removeInstrument(int id) {
    instruments_.erase(
        std::remove_if(instruments_.begin(), instruments_.end(),
            [id](const Instrument& i) { return i.getId() == id; }),
        instruments_.end()
    );
}

Instrument* InstrumentRack::getInstrument(int id) {
    auto it = std::find_if(instruments_.begin(), instruments_.end(),
        [id](const Instrument& i) { return i.getId() == id; });
    
    return (it != instruments_.end()) ? &(*it) : nullptr;
}

const Instrument* InstrumentRack::getInstrument(int id) const {
    auto it = std::find_if(instruments_.begin(), instruments_.end(),
        [id](const Instrument& i) { return i.getId() == id; });
    
    return (it != instruments_.end()) ? &(*it) : nullptr;
}

bool InstrumentRack::hasInstrument(int id) const {
    return getInstrument(id) != nullptr;
}

void InstrumentRack::clear() {
    instruments_.clear();
}

int InstrumentRack::getNextId() const {
    if (instruments_.empty()) {
        return 1;
    }
    
    // Find max ID and return next
    int maxId = 0;
    for (const auto& inst : instruments_) {
        if (inst.getId() > maxId) {
            maxId = inst.getId();
        }
    }
    
    return maxId + 1;
}

} // namespace beater
