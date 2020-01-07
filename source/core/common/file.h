#pragma once

#include <string>

namespace file
{
    extern std::string getNameFromPath(const std::string& path);
    extern std::string stripExtension(const std::string& fileName);
    extern std::string createFullPath(const std::string& fileName, const std::string& extension, const std::string& path);
}