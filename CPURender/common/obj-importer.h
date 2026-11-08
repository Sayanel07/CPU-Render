#pragma once

#include <vector>
#include <string>

#include "geometry.h"

class ObjImporter {
public:
    static Geometry loadFromFile(const std::string& path);
};