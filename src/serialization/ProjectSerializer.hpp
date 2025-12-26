#pragma once

#include "domain/Project.hpp"
#include <string>

namespace beater {

// Project serializer (stub for now)
class ProjectSerializer {
public:
    static bool saveToFile(const Project& project, const std::string& filepath);
    static bool loadFromFile(Project& project, const std::string& filepath);
};

} // namespace beater
