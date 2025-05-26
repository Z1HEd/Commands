#pragma once
#include "CommandException.h"

namespace commandExceptions {
	class UnknownEntityException : public CommandException{
	public:
		inline UnknownEntityException(const std::string& name) : CommandException(std::format("Unknown Entity: {}", name)){}
	};
}