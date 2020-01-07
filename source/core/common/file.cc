#include "file.h"

// Some helper definitions
#if defined(_WIN32)
static const std::string FILE_SEPARATOR = "\\";
#elif defined(__linux)
static const std::string FILE_SEPARATOR = "/";
#endif

std::string file::getNameFromPath(const std::string& path)
{
    size_t indexSeparator = path.rfind(FILE_SEPARATOR);
    if (indexSeparator == std::string::npos)
    {
        return path;
    }
    else
    {
        return path.substr(indexSeparator + 1);
    }
}

std::string file::stripExtension(const std::string& fileName)
{
    size_t indexExtension = fileName.find_last_of(".");
    if (indexExtension != std::string::npos)
    {
        return fileName.substr(0, indexExtension);
    }
    else
    {
        return fileName;
    }
}

std::string file::createFullPath(const std::string& fileName, const std::string& extension, const std::string& path)
{
    if (path.empty())
    {
        return fileName + "." + extension;
    }
    else
    {
        return path + FILE_SEPARATOR + fileName + "." + extension;
    }
}