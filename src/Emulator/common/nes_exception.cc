#include "nes_exception.h"

NesException::NesException(const std::string& component, const std::string& message)
    : _message(component + ": " + message)
{
}

const char* NesException::what() const noexcept
{
    return _message.c_str();
}