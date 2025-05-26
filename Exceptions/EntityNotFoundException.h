#pragma once
#include "CommandException.h"

namespace commandExceptions {
	class EntityNotFoundException : public CommandException {
	public:
		inline EntityNotFoundException(const std::string& name) : CommandException(std::format("Entity not found: {}", name)) {}
	};
}