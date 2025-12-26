#include "domain/Region.hpp"

namespace beater {

Region::Region(const std::string& id, RegionType type, Tick startTick, Tick lengthTicks)
    : id_(id), type_(type), startTick_(startTick), lengthTicks_(lengthTicks) {
}

} // namespace beater
