#pragma once
#include <exception>
#include "format"

namespace commandExceptions {
    class CommandException : public std::exception {
    public:
        inline const char* what() const {
            return message.c_str();
        }
        inline CommandException() : message("Unknown command exception") {}
        inline CommandException(const std::string& exception) : message(exception) {}
    protected:
        std::string message;
    };
}