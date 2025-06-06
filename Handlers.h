#pragma once
#include <4dm.h>
#include "utils.h"
#include "Aliases.h"
#include "Command.h"
#include "Parsing.h"

using namespace fdm;
using namespace utils;
using namespace aliases;
using namespace parsing;
using namespace commandExceptions;

std::string tpHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {
	glm::vec4 position{};
	std::vector<Entity*> targets;

	switch (parameters.size()) {
	case 1: {
		// /tp <target>
		targets.push_back( parseEntity(parameters[0], player, world));
		position = targets[0]->getPos();
		position += glm::vec4(0.001f);
		setEntityPosition(world->getEntity(player->EntityPlayerID), position, world);
		return std::format("Teleported you to {}", getEntityName(targets[0]));
	}
	case 2: {
		// /tp <entity-list> <destination-entity>
		targets = parseEntityList(parameters[0], player, world);
		Entity* dest = parseEntity(parameters[1], player, world);
		position = dest->getPos();
		position += glm::vec4(0.001f);
		for (auto* e : targets)
			setEntityPosition(e, position, world);
		return std::format("Teleported {} entities to {}",
			targets.size(), getEntityName(dest));
	}
	case 4: {
		// /tp <x> <y> <z> <w>
		position = parsePosition(parameters, 0, player);
		position += glm::vec4(0.001f);
		setEntityPosition(world->getEntity(player->EntityPlayerID), position, world);
		return std::format("Teleported you to {:.1f},{:.1f},{:.1f},{:.1f}",
			position.x, position.y, position.z, position.w);
	}
	case 5: {
		// /tp <entity-list> <x> <y> <z> <w>
		targets = parseEntityList(parameters[0], player, world);
		position = parsePosition(parameters, 1, player);
		position += glm::vec4(0.001f);
		for (auto* e : targets)
			setEntityPosition(e, position, world);
		return std::format("Teleported {} entities to {:.1f},{:.1f},{:.1f},{:.1f}",
			targets.size(),
			position.x, position.y, position.z, position.w);
	}
	}
	return "this will never happen, compiler shut up";
}

std::string killHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {

	const std::string& entityString = parameters[0];
	auto entities = parseEntityList(entityString, player, world);

	for (auto* e : entities) {
		killEntity(e, world);
	}

	if (entities.size() == 1) {
		return std::format("Killed {}", getEntityName(entities[0]));
	}
	else {
		return std::format("Killed {} entities", entities.size());
	}
}

std::string fillHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {
	constexpr int maxFillSize = 32 * 32 * 32 * 32;

	auto [startPosition, endPosition] = parseArea(parameters, 0, player);
	int blockType = parseInt(parameters[8]);

	int sizeX = endPosition.x - startPosition.x + 1;
	int sizeY = endPosition.y - startPosition.y + 1;
	int sizeZ = endPosition.z - startPosition.z + 1;
	int sizeW = endPosition.w - startPosition.w + 1;
	long long areaSize = 1LL * sizeX * sizeY * sizeZ * sizeW;

	if (areaSize > maxFillSize) {
		throw ConstraintException(
			std::format("Cannot fill {} blocks, max amount is {}", areaSize, maxFillSize)
		);
	}

	for (int x = startPosition.x; x <= endPosition.x; ++x) {
		for (int y = startPosition.y; y <= endPosition.y; ++y) {
			for (int z = startPosition.z; z <= endPosition.z; ++z) {
				for (int w = startPosition.w; w <= endPosition.w; ++w) {
					glm::ivec4 pos{ x, y, z, w };
					world->setBlockUpdate(pos, blockType);
					if (blockType == BlockInfo::CHEST) {
						nlohmann::json chestAttributes = {
							{"type", "chest"},
							{"attributes", {
								{"inventory", std::vector<nlohmann::json>(32, nlohmann::json::object())}
							}}
						};
						auto chest = Entity::createFromJson(chestAttributes);
						world->addEntityToChunk(chest, world->getChunk(pos));
					}
				}
			}
		}
	}

	return std::format("Filled {} blocks", areaSize);
}

std::string cloneHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {
	constexpr int maxAreaSize = 32 * 32 * 32 * 32;

	auto [fromStart, fromEnd] = parseArea(parameters, 0, player);
	glm::ivec4 toStart = parsePosition(parameters, 8, player);

	int sizeX = fromEnd.x - fromStart.x + 1;
	int sizeY = fromEnd.y - fromStart.y + 1;
	int sizeZ = fromEnd.z - fromStart.z + 1;
	int sizeW = fromEnd.w - fromStart.w + 1;
	long long areaSize = sizeX * sizeY * sizeZ * sizeW;

	if (areaSize > maxAreaSize) {
		throw ConstraintException(
			std::format("Cannot clone {} blocks, max amount is {}", areaSize, maxAreaSize)
		);
	}

	std::vector<uint8_t> blocks(areaSize);
	auto flatten = [&](int x, int y, int z, int w) {
		return ((x * sizeY + y) * sizeZ + z) * sizeW + w;
		};

	for (int x = 0; x < sizeX; ++x) {
		for (int y = 0; y < sizeY; ++y) {
			for (int z = 0; z < sizeZ; ++z) {
				for (int w = 0; w < sizeW; ++w) {
					blocks[flatten(x, y, z, w)] =
						world->getBlock(fromStart + glm::ivec4{ x,y,z,w });
				}
			}
		}
	}

	for (int x = 0; x < sizeX; ++x) {
		for (int y = 0; y < sizeY; ++y) {
			for (int z = 0; z < sizeZ; ++z) {
				for (int w = 0; w < sizeW; ++w) {
					glm::ivec4 dst = toStart + glm::ivec4{ x,y,z,w };
					uint8_t id = blocks[flatten(x, y, z, w)];
					world->setBlockUpdate(dst, id);
					if (id == BlockInfo::CHEST) {
						nlohmann::json chestAttributes = {
							{"type", "chest"},
							{"attributes", {
								{"inventory", std::vector<nlohmann::json>(32, nlohmann::json::object())}
							}}
						};
						auto chest = Entity::createFromJson(chestAttributes);
						world->addEntityToChunk(chest, world->getChunk(dst));
					}
				}
			}
		}
	}

	return std::format("Cloned {} blocks", areaSize);
}

std::string seedHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {

	if (world->getType() == World::TYPE_SINGLEPLAYER) {
		auto* sp = static_cast<WorldSingleplayer*>(world);
		return std::format("Seed: {}", sp->chunkLoader.seed);
	}

	auto* sp = static_cast<WorldServer*>(world);
	return std::format("Seed: {}", sp->chunkLoader.seed);
}

std::string spawnHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {

	const std::string& entityName = parameters[0];
	glm::vec4 position = parsePosition(parameters, 1, player);

	spawnEntity(entityName, position);

	return std::format("Spawned {}", entityName);
}

std::string difficultyHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {

	if (parameters.empty()) {
		int current = world->getType() == World::TYPE_SINGLEPLAYER ?
			((WorldSingleplayer*)world)->chunkLoader.difficulty :
			((WorldServer*)world)->chunkLoader.difficulty;
		return std::format("Current difficulty: {}", current);
	}

	int difficulty = parseInt(parameters[0]);
	if (difficulty < 0 || difficulty > 2) {
		throw ConstraintException(
			std::format("Invalid difficulty: {}! Must be 0, 1, or 2", difficulty)
		);
	}
	if (world->getType() == World::TYPE_SINGLEPLAYER) {
		((WorldSingleplayer*)world)->chunkLoader.difficulty = difficulty;
		StateSettings::instanceObj.updateDifficulty(difficulty);
		StateSettings::instanceObj.save();
	}
	else
		((WorldServer*)world)->chunkLoader.difficulty = difficulty; // I hope I dont have to do anything else

	return std::format("Set difficulty to: {}", difficulty);
}

std::string giveHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {
	// /give <item>
	if (parameters.size() == 1) {
		std::string itemName = parseText(parameters[0]);
		spawnEntityItem(itemName, 1, player->pos, world);
		return std::format("Given item: {} x1", itemName);
	}

	// /give <something> <something>
	if (parameters.size() == 2) {
		// /give <item> <count>
		try {
			int count = parseInt(parameters[1]);
			std::string itemName = parseText(parameters[0]);
			spawnEntityItem(itemName, count, player->pos, world);
			return std::format("Given item: {} x{}", itemName, count);
		}
		catch (const ParsingException&) {
			// /give <entity> <item>
			std::string entityString = parameters[0];
			std::string itemName = parseText(parameters[1]);
			auto entities = parseEntityList(entityString, player, world);

			for (auto* e : entities) {
				spawnEntityItem(itemName, 1, e->getPos(), world);
			}
			return std::format("Given item: {} x1 to {} entities", itemName, entities.size());
		}
	}

	// /give <entity> <item> <count>
	{
		std::string entityString = parameters[0];
		std::string itemName = parseText(parameters[1]);
		int count = parseInt(parameters[2]);

		auto entities = parseEntityList(entityString, player, world);

		for (auto* e : entities) {
			spawnEntityItem(itemName, count, e->getPos(), world);
		}
		return std::format(
			"Given item: {} x{} to {} entities",
			itemName,
			count,
			entities.size()
		);
	}
}

std::string rotateHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {
	const std::string& modeRaw = parseText(parameters[0]);

	// strip and uppercase base mode
	std::string m = modeRaw;
	bool is180 = false;
	if (m.size() > 2 && m.substr(m.size() - 3) == "180") {
		is180 = true;
		m.erase(m.size() - 3);
	}
	for (char& c : m) c = char(std::toupper(c));

	// validate plane
	static constexpr const char* canon[] = { "XY","XZ","XW","YZ","YW","ZW" };
	std::string ab = (std::find(std::begin(canon), std::end(canon), m) != std::end(canon)
		? m : std::string{ m[1],m[0] });
	if (std::find(std::begin(canon), std::end(canon), ab) == std::end(canon)) {
		throw ConstraintException(std::format(
			"Unknown rotate plane: '{}'.", modeRaw));
	}

	auto [start, end] = parseArea(parameters, 0, player);

	// if safe mode, enforce square area in that plane
	if (!modifiers.contains("unsafe")) {
		auto axisIndex = [&](char C) {
			return C == 'X' ? 0 : C == 'Y' ? 1 : C == 'Z' ? 2 : 3;
			};
		int ia = axisIndex(m[0]), ib = axisIndex(m[1]);
		int lenA = std::abs(end[ia] - start[ia]);
		int lenB = std::abs(end[ib] - start[ib]);
		if (lenA != lenB) {
			throw ConstraintException(std::format(
				"Rotation area must be square in plane {}.\n"
				"Rotating non-square areas is unsafe and can destroy blocks around it.\n"
				"Use 'unsafe' modifier to bypass this check.", m
			));
		}
	}

	auto count = rotateArea(modeRaw, start, end, world);
	return std::format("Rotated {} blocks ({})", count, modeRaw);
}

std::string mirrorHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {

	std::string axesRaw = parseText(parameters[0]);
	for (auto& c : axesRaw) c = std::toupper(c);
	if (axesRaw.empty() || axesRaw.size() > 4) {
		throw ConstraintException("Mirror axes string must be 1�4 characters long.");
	}

	bool mirrorAxis[4] = { false,false,false,false };
	for (char c : axesRaw) {
		switch (c) {
		case 'X': mirrorAxis[0] = true; break;
		case 'Y': mirrorAxis[1] = true; break;
		case 'Z': mirrorAxis[2] = true; break;
		case 'W': mirrorAxis[3] = true; break;
		default:
			throw ConstraintException(std::format(
				"Invalid mirror axis '{}'. Valid axes are X, Y, Z, W.", c
			));
		}
	}

	auto [start, end] = parseArea(parameters, 1, player);

	std::size_t count = mirrorArea(mirrorAxis, start, end, world);
	return std::format("Mirrored {} blocks on axes {}",
		count, axesRaw);
}

std::string aliasHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* caller,
	World* world
) {
	if (modifiers.contains("remove")) {
		assertArgumentCount(parameters, { 1 });

		std::string key = parseText(parameters[0]);
		if (!aliasMap.contains(key))
			throw AliasException(std::format("Tried removing unknown alias: {}", key));

		aliasMap.erase(key);
		return std::format("Alias '{}' has been removed", key);
	}

	if (parameters.empty()) {
		std::string result = "Known aliases:";
		for (const auto& [key, val] : aliasMap)
			result += std::format("\n'{}' => '{}'", key, val);
		return result;
	}

	std::string key = parseText(parameters[0]);

	if (parameters.size() == 1) {
		if (!aliasMap.contains(key))
			throw AliasException(std::format("Alias '{}' is not defined.", key));
		return std::format("Alias '{}' => {}", key, aliasMap[key]);
	}

	std::string value = parameters[1];
	for (size_t i = 2; i < parameters.size(); ++i)
		value += " " + parameters[i];

	if (aliasMap.contains(key)) {
		if (!modifiers.contains("overwrite"))
			throw AliasException(std::format("Alias '{}' is already defined.", key));
		aliasMap[key] = value;
		saveAliases();
		return std::format("Alias '{}' has been overwritten to '{}'", key, value);
	}
	else {
		addAlias(key, value);
		saveAliases();
		return std::format("Added alias: '{}' => '{}'", key, value);
	}
}

std::string helpHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* caller,
	World* world
) {

	if (parameters.empty()) {
		std::ostringstream oss;
		oss << "Available commands:\n";
		for (const auto& [name, cmd] : commands) {
			oss << "  /" << name << " - " << cmd.description << "\n";
		}
		return oss.str();
	}
	else {
		const std::string& cmdName = parseText(parameters[0]);
		if (!commands.contains(cmdName)) {
			throw CommandException(std::format("Unknown command '{}'.", cmdName));
		}
		const Command& cmd = commands[cmdName];

		return std::format("Usage for '{}':\n{}", cmdName, cmd.usage);
	}
}

std::string clearHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* caller,
	World* world
) {
	StateGame::instanceObj.chatMessageContainer.clear();
	return "";
}