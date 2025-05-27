#pragma once
#include <4dm.h>
#include "utils.h"


using namespace fdm;
using namespace utils;
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
		targets = getEntities(parameters[0], player, world);
		if (targets.empty())
			throw EntityNotFoundException(parameters[0]);
		if (targets.size() > 1)
			throw MultipleEntitiesException("First parameter",targets.size());
		position = targets[0]->getPos();
		position += glm::vec4(0.001f);
		setEntityPosition(world->getEntity(player->EntityPlayerID), position,world);
		return std::format("Teleported you to {}", getEntityName(targets[0]));
	}
	case 2: {
		// /tp <entity-list> <destination-entity>
		targets = getEntities(parameters[0], player, world);
		if (targets.empty())
			throw EntityNotFoundException(parameters[0]);
		std::vector<Entity*> dest = getEntities(parameters[1], player, world);
		if (dest.empty())
			throw EntityNotFoundException(parameters[1]);
		if (dest.size() > 1)
			throw MultipleEntitiesException("Second parameter",dest.size());
		position = dest[0]->getPos();
		position += glm::vec4(0.001f);
		for (auto* e : targets)
			setEntityPosition(e, position, world);
		return std::format("Teleported {} entities to {}",
			targets.size(), getEntityName(dest[0]));
	}
	case 4: {
		// /tp <x> <y> <z> <w>
		position.x = parseFloat(parameters[0]);
		position.y = parseFloat(parameters[1]);
		position.z = parseFloat(parameters[2]);
		position.w = parseFloat(parameters[3]);
		position += glm::vec4(0.001f);
		setEntityPosition(world->getEntity(player->EntityPlayerID), position, world);
		return std::format("Teleported you to {:.1f},{:.1f},{:.1f},{:.1f}",
			position.x, position.y, position.z, position.w);
	}
	case 5: {
		// /tp <entity-list> <x> <y> <z> <w>
		targets = getEntities(parameters[0], player, world);
		if (targets.empty())
			throw EntityNotFoundException(parameters[0]);
		position.x = parseFloat(parameters[1]);
		position.y = parseFloat(parameters[2]);
		position.z = parseFloat(parameters[3]);
		position.w = parseFloat(parameters[4]);
		position += glm::vec4(0.001f);
		for (auto* e : targets)
			setEntityPosition(e, position, world);
		return std::format("Teleported {} entities to {:.1f},{:.1f},{:.1f},{:.1f}",
			targets.size(),
			position.x, position.y, position.z, position.w);
	}
	default:
		throw ArgumentCountException(parameters.size(), {1,2,4,5});
	}
}

std::string killHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {
	if (parameters.size() != 1) {
		throw ArgumentCountException(parameters.size(), { 1 });
	}

	const std::string& entityString = parameters[0];
	auto entities = getEntities(entityString, player, world);

	if (entities.empty()) {
		throw EntityNotFoundException(entityString);
	}

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
	if (parameters.size() != 9) {
		throw ArgumentCountException(parameters.size(), { 9 });
	}

	glm::ivec4 startPosition{
		parseInt(parameters[0]),
		parseInt(parameters[1]),
		parseInt(parameters[2]),
		parseInt(parameters[3])
	};
	glm::ivec4 endPosition{
		parseInt(parameters[4]),
		parseInt(parameters[5]),
		parseInt(parameters[6]),
		parseInt(parameters[7])
	};
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
	if (parameters.size() != 12) {
		throw ArgumentCountException(parameters.size(), { 12 });
	}

	glm::ivec4 fromStart{
		parseInt(parameters[0]),
		parseInt(parameters[1]),
		parseInt(parameters[2]),
		parseInt(parameters[3])
	};
	glm::ivec4 fromEnd{
		parseInt(parameters[4]),
		parseInt(parameters[5]),
		parseInt(parameters[6]),
		parseInt(parameters[7])
	};
	glm::ivec4 toStart{
		parseInt(parameters[8]),
		parseInt(parameters[9]),
		parseInt(parameters[10]),
		parseInt(parameters[11])
	};

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
	if (parameters.size() != 0) {
		throw ArgumentCountException(parameters.size(), { 0 });
	}
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
	if (parameters.size() != 5) {
		throw ArgumentCountException(parameters.size(), { 5 });
	}

	const std::string& entityName = parameters[0];
	glm::vec4 position{
		parseFloat(parameters[1]),
		parseFloat(parameters[2]),
		parseFloat(parameters[3]),
		parseFloat(parameters[4])
	};

	spawnEntity(entityName, position);

	return std::format("Spawned {}", entityName);
}

std::string difficultyHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {
	if (parameters.size() > 1) {
		throw ArgumentCountException(parameters.size(), { 0, 1 });
	}

	// No parameters: report current difficulty
	if (parameters.empty()) {
		int current = world->getType() == World::TYPE_SINGLEPLAYER ?
			((WorldSingleplayer*)world)->chunkLoader.difficulty :
			((WorldServer*)world)->chunkLoader.difficulty;
		return std::format("Current difficulty: {}", current);
	}

	// One parameter: parse and validate
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
	if (parameters.size() < 1 || parameters.size() > 3) {
		throw ArgumentCountException(parameters.size(), { 1, 2, 3 });
	}

	auto replaceUnderscores = [&](std::string s) {
		std::replace(s.begin(), s.end(), '_', ' ');
		return s;
		};

	// /give <item>
	if (parameters.size() == 1) {
		std::string itemName = replaceUnderscores(parameters[0]);
		spawnEntityItem(itemName, 1, player->pos, world);
		return std::format("Given item: {} x1", itemName);
	}

	// /give <something> <something>
	if (parameters.size() == 2) {
		// /give <item> <count>
		try {
			int count = parseInt(parameters[1]);
			std::string itemName = replaceUnderscores(parameters[0]);
			spawnEntityItem(itemName, count, player->pos, world);
			return std::format("Given item: {} x{}", itemName, count);
		}
		catch (const ParsingException&) {
			// /give <entity> <item>
			std::string entityString = parameters[0];
			std::string itemName = replaceUnderscores(parameters[1]);
			auto entities = getEntities(entityString, player, world);
			if (entities.empty()) {
				throw EntityNotFoundException(entityString);
			}
			for (auto* e : entities) {
				spawnEntityItem(itemName, 1, e->getPos(), world);
			}
			return std::format("Given item: {} x1 to {} entities", itemName, entities.size());
		}
	}

	// /give <entity> <item> <count>
	{
		std::string entityString = parameters[0];
		std::string itemName = replaceUnderscores(parameters[1]);
		int count = parseInt(parameters[2]);

		auto entities = getEntities(entityString, player, world);
		if (entities.empty()) {
			throw EntityNotFoundException(entityString);
		}
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

static std::size_t rotateArea(
	const std::string& mode,
	const glm::ivec4& start,
	const glm::ivec4& end,
	World* world
) {
	// compute center
	glm::vec4 center = (glm::vec4(start) + glm::vec4(end)) * 0.5f;

	// harvest blocks & chests
	struct BlockRec { glm::ivec4 pos; uint8_t id; };
	std::vector<BlockRec> blocks;
	blocks.reserve(
		(end.x - start.x + 1) *
		(end.y - start.y + 1) *
		(end.z - start.z + 1) *
		(end.w - start.w + 1)
	);
	struct ChestRec { EntityChest* ent; glm::ivec4 pos; };
	std::vector<ChestRec> chests;
	for (int x = start.x; x <= end.x; ++x)
		for (int y = start.y; y <= end.y; ++y)
			for (int z = start.z; z <= end.z; ++z)
				for (int w = start.w; w <= end.w; ++w) {
					glm::ivec4 p{ x,y,z,w };
					uint8_t id = world->getBlock(p);
					blocks.push_back({ p,id });
					if (id == BlockInfo::CHEST) {
						if (auto* e = (EntityChest*)world->entities.getBlockEntity(p))
							chests.push_back({ e,p });
					}
				}

	// clear original region
	for (auto& br : blocks)
		world->setBlockUpdate(br.pos, BlockInfo::AIR);

	// find axes to rotate
	auto axisIndex = [&](char C) {
		return C == 'X' ? 0 : C == 'Y' ? 1 : C == 'Z' ? 2 : 3;
		};
	bool is180 = false, reverse = false;
	// parse "180" and reverse from mode string
	std::string m = mode;
	if (m.size() > 2 && m.substr(m.size() - 3) == "180") {
		is180 = true;
		m.erase(m.size() - 3);
	}
	char A = char(std::toupper(m[0])), B = char(std::toupper(m[1]));
	static const char* canon[] = { "XY","XZ","XW","YZ","YW","ZW" };
	if (std::find(std::begin(canon), std::end(canon), m) == std::end(canon)) {
		std::swap(A, B);
		reverse = true;
	}
	int ia = axisIndex(A), ib = axisIndex(B);

	// rotate and write back
	auto rotatePt = [&](const glm::ivec4& ipos) {
		glm::vec4 f = glm::vec4(ipos) - center;
		float a = f[ia], b = f[ib], na, nb;
		if (is180) { na = -a;       nb = -b; }
		else if (!reverse) { na = -b;       nb = a; } // CCW
		else /* reverse */ { na = b;       nb = -a; } // CW
		f[ia] = na; f[ib] = nb;
		glm::vec4 fr = f + center;
		return glm::ivec4{
			glm::round(fr.x),
			glm::round(fr.y),
			glm::round(fr.z),
			glm::round(fr.w)
		};
		};

	for (auto& br : blocks) {
		glm::ivec4 np = rotatePt(br.pos);
		world->setBlockUpdate(np, br.id);
	}
	for (auto& cr : chests) {
		glm::ivec4 np = rotatePt(cr.pos);
		cr.ent->setPos(np);
	}
	world->entities.relocateBlockEntities();
	return blocks.size();
}

std::string rotateHandle(
	const std::unordered_set<std::string>& modifiers,
	const std::vector<std::string>& parameters,
	Player* player,
	World* world
) {
	// expect 9 parameters: mode + 8 coords
	if (parameters.size() != 9) {
		throw ArgumentCountException(parameters.size(), { 9 });
	}

	const std::string& modeRaw = parameters[0];

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

	// parse coordinates
	glm::ivec4 start{
		parseInt(parameters[1]),
		parseInt(parameters[2]),
		parseInt(parameters[3]),
		parseInt(parameters[4])
	};
	glm::ivec4 end{
		parseInt(parameters[5]),
		parseInt(parameters[6]),
		parseInt(parameters[7]),
		parseInt(parameters[8])
	};

	// if safe mode, enforce square area in that plane
	if (!modifiers.count("unsafe")) {
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