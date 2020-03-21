#pragma once

#include <string>
#include <exception>

class NesException : public std::exception
{
public:
    explicit NesException(const std::string& component, const std::string& message);
    const char* what() const noexcept override;

private:
    std::string _message;
};