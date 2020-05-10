#pragma once

#include <exception>

#include "error_handling.h"

class NesException : public std::exception
{
public:
    explicit NesException(int errorCode): _errorCode(errorCode) {} 

    const char* what() const noexcept override
    {
        return ::GetErrorMessageFromCode(_errorCode);
    }

    int errorCode() const noexcept { return _errorCode; };

private:
    int _errorCode;
};