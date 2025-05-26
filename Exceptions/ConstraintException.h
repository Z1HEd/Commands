#pragma once
#include "CommandException.h"

namespace commandExceptions {
	class ConstraintException : public CommandException {
	public:
		inline ConstraintException(const std::string& info) : CommandException(info) {}
	};
}