#pragma once
#include <4dm.h>


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
    
    class CommandSyntaxException : public CommandException {
    public:
        inline CommandSyntaxException(const std::string& input, const std::string& rule )
            : CommandException(std::format("Invalid syntax: {} ({})",input,rule)) {
        }
    };

    class ArgumentCountException : public CommandException {
        inline static std::string formatMessage(
            const int& receivedCount,
            const std::set<int>& expectedCounts
        ) {
            std::ostringstream oss;
            oss << "Invalid number of arguments: received "
                << receivedCount
                << ", expected ";

            for (auto it = expectedCounts.begin(); it != expectedCounts.end(); ++it) {
                if (it != expectedCounts.begin()) {
                    if (std::next(it) == expectedCounts.end()) 
                        oss << " or ";
                    else 
                        oss << ", ";
                }
                oss << *it;
            }

            oss << ".";
            return oss.str();
        }

    public:
        inline ArgumentCountException(
            const int& receivedCount,
            const std::set<int>& expectedCounts
        )
            : CommandException(formatMessage(receivedCount, expectedCounts))
        {
        }
    };
    
    class AliasException : public CommandException {
    public:
        inline AliasException(const std::string& message) : CommandException(message) {}
    };
    
    class ConstraintException : public CommandException {
    public:
        ConstraintException(const std::string& info) : CommandException(info) {}
    };
    
    class EntityNotFoundException : public CommandException {
    public:
        inline EntityNotFoundException(const std::string& name) : CommandException(std::format("Entity not found: {}", name)) {}
    };

    class MultipleEntitiesException : public CommandException {
    public:
        MultipleEntitiesException(const std::string& source, int countReceived) : CommandException(std::format("{}: excpected single entity, received {}", source, countReceived)) {}
    };

    class ParsingException : public CommandException {
    public:
        ParsingException(const std::string& receivedString, const std::string& expectedTypeName)
            : CommandException(std::format("Failed to parse '{}' to {}", receivedString, expectedTypeName)) {
        }
    };

    class UnknownEntityException : public CommandException {
    public:
        UnknownEntityException(const std::string& name) : CommandException(std::format("Unknown Entity: {}", name)) {}
    };

    class UnknownItemException : public CommandException {
    public:
        UnknownItemException(const std::string& name) : CommandException(std::format("Unknown Item: {}", name)) {}
    };
}