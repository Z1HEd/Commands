#pragma once
#include "CommandException.h"

namespace commandExceptions {
	class UnknownItemException : public CommandException{
	public:
		inline UnknownItemException(const std::string& name) : CommandException(std::format("Unknown Item: {}", name)) {}
	};
}