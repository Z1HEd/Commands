#pragma once
#include <4dm.h>
#include "CommandExceptions.h"

using namespace fdm;
using namespace commandExceptions;

using commandHandler = std::string(*)(
	const std::unordered_set<std::string>&,
	const std::vector<std::string>&,
	Player*,
	World*);

class Command {
public:
	std::string name;
	commandHandler handler;
	std::string description;
	std::string usage;
	std::set<int> argumentCounts;

	std::string handle(const std::unordered_set<std::string>& modifiers,
		const std::vector<std::string>& arguments,
		Player* player, World* world) {
		if (!argumentCounts.contains(arguments.size()))
			throw ArgumentCountException(arguments.size(), argumentCounts);
		return handler(modifiers, arguments, player, world);
	}
};

extern std::unordered_map<std::string, Command> commands;