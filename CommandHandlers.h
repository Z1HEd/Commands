#pragma once
#include <4dm.h>
#include "utils.h"


using namespace fdm;
using namespace utils;
using namespace commandExceptions;

std::string tpHandle(std::stringstream& stream, Player* player, World* world) {
	glm::vec4 position = {};

	std::string entityString;
	stream >> entityString;

	stream >> position.x;
	stream >> position.y;
	stream >> position.z;
	stream >> position.w;

	position += 0.001; // 4D Miner physics doesnt like exact coordinates

	std::vector<Entity*> entities = getEntities(entityString, player, world);
	for (auto entity : entities) {
		setEntityPosition(entity, position);
	}


	return entities.size() == 1 ?
		std::format("Teleported {} to {:.1f},{:.1f},{:.1f},{:.1f}", getEntityName(entities[0]),
			((EntityPlayer*)entities[0])->player->pos.x,
			((EntityPlayer*)entities[0])->player->pos.y,
			((EntityPlayer*)entities[0])->player->pos.z,
			((EntityPlayer*)entities[0])->player->pos.w) :
		std::format("Teleported {} entities to {:.1f},{:.1f},{:.1f},{:.1f}", entities.size(),
			((EntityPlayer*)entities[0])->player->pos.x,
			((EntityPlayer*)entities[0])->player->pos.y,
			((EntityPlayer*)entities[0])->player->pos.z,
			((EntityPlayer*)entities[0])->player->pos.w);
}

std::string killHandle(std::stringstream& stream, Player* player, World* world) {
	std::string entityString;
	stream >> entityString;
	std::vector<Entity*> entities = getEntities(entityString, player, world);
	for (auto entity : entities) {
		killEntity(entity, world);
	}
	return entities.size() == 1 ? std::format("Killed {}", getEntityName(entities[0])) :
		std::format("Killed {} entities", entities.size());
}

std::string fillHandle(std::stringstream& stream, Player* player, World* world) {
	constexpr int maxFillSize = 32 * 32 * 32 * 32;
	glm::ivec4 startPosition;
	glm::ivec4 endPosition;

	int blockType;

	stream >> startPosition.x;
	stream >> startPosition.y;
	stream >> startPosition.z;
	stream >> startPosition.w;

	stream >> endPosition.x;
	stream >> endPosition.y;
	stream >> endPosition.z;
	stream >> endPosition.w;

	stream >> blockType;

	int sizeX = endPosition.x - startPosition.x + 1;
	int sizeY = endPosition.y - startPosition.y + 1;
	int sizeZ = endPosition.z - startPosition.z + 1;
	int sizeW = endPosition.w - startPosition.w + 1;

	int areaSize = sizeX * sizeY * sizeZ * sizeW;

	if (areaSize > maxFillSize)
		return std::format("Cannot fill {} blocks, max amount is {}", areaSize, maxFillSize);

	for (int x = startPosition.x;x <= endPosition.x;x++)
		for (int y = startPosition.y;y <= endPosition.y;y++)
			for (int z = startPosition.z;z <= endPosition.z;z++)
				for (int w = startPosition.w;w <= endPosition.w;w++) {
					world->setBlockUpdate(glm::ivec4{ x,y,z,w }, blockType);
					if (blockType == BlockInfo::CHEST) {
						nlohmann::json chestAttributes = {
							{"type", "chest"},
							{"attributes" , {
								"inventory", {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},}
								}
							}
						};
						auto chest = Entity::createFromJson(chestAttributes);
						world->addEntityToChunk(chest, world->getChunk(glm::ivec4{ x,y,z,w }));
					}
				}
	return std::format("Filled {} blocks", areaSize);
}

std::string cloneHandle(std::stringstream& stream, Player* player, World* world) {
	constexpr int maxAreaSize = 32 * 32 * 32 * 32;
	glm::ivec4 fromStartPosition;
	glm::ivec4 fromEndPosition;
	glm::ivec4 toStartPosition;

	stream >> fromStartPosition.x;
	stream >> fromStartPosition.y;
	stream >> fromStartPosition.z;
	stream >> fromStartPosition.w;

	stream >> fromEndPosition.x;
	stream >> fromEndPosition.y;
	stream >> fromEndPosition.z;
	stream >> fromEndPosition.w;

	stream >> toStartPosition.x;
	stream >> toStartPosition.y;
	stream >> toStartPosition.z;
	stream >> toStartPosition.w;

	int sizeX = fromEndPosition.x - fromStartPosition.x + 1;
	int sizeY = fromEndPosition.y - fromStartPosition.y + 1;
	int sizeZ = fromEndPosition.z - fromStartPosition.z + 1;
	int sizeW = fromEndPosition.w - fromStartPosition.w + 1;

	int areaSize = sizeX * sizeY * sizeZ * sizeW;

	if (areaSize > maxAreaSize)
		return std::format("Cannot clone {} blocks, max amount is {}", areaSize, maxAreaSize);

	uint8_t**** blockIDs = new uint8_t * **[sizeX];
	for (int x = 0; x < sizeX; ++x) {
		blockIDs[x] = new uint8_t * *[sizeY];
		for (int y = 0; y < sizeY; ++y) {
			blockIDs[x][y] = new uint8_t * [sizeZ];
			for (int z = 0; z < sizeZ; ++z) {
				blockIDs[x][y][z] = new uint8_t[sizeW];
			}
		}
	}
	for (int x = 0; x < sizeX; ++x)
		for (int y = 0; y < sizeY; ++y)
			for (int z = 0; z < sizeZ; ++z)
				for (int w = 0; w < sizeW; ++w)
					blockIDs[x][y][z][w] = world->getBlock(fromStartPosition + glm::ivec4(x, y, z, w));

	for (int x = 0; x < sizeX; ++x)
		for (int y = 0; y < sizeY; ++y)
			for (int z = 0; z < sizeZ; ++z)
				for (int w = 0; w < sizeW; ++w)
				{
					world->setBlockUpdate(toStartPosition + glm::ivec4(x, y, z, w), blockIDs[x][y][z][w]);
					if (blockIDs[x][y][z][w] == BlockInfo::CHEST) {
						nlohmann::json chestAttributes = {
							{"type", "chest"},
							{"attributes" , {
								"inventory", {{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},}
								}
							}
						};
						auto chest = Entity::createFromJson(chestAttributes);
						world->addEntityToChunk(chest, world->getChunk(glm::ivec4{ x,y,z,w }));
					}
				}

	return std::format("Cloned {} blocks", areaSize);
}

std::string seedHandleSingleplayer(std::stringstream& stream, Player* player, World* world) {
	if (world->getType() != World::TYPE_SINGLEPLAYER)
		return "Can only use seed command in singleplayer!";
	return std::format("Seed: {}", ((WorldSingleplayer*)world)->chunkLoader.seed);
}

std::string spawnHandle(std::stringstream& stream, Player* player, World* world) {
	std::string entityName;
	glm::vec4 position;
	stream >> entityName;

	stream >> position.x;
	stream >> position.y;
	stream >> position.z;
	stream >> position.w;
	try {
		spawnEntity(entityName, position);
	}
	catch (const UnknownEntityException& ex) {
		return ex.what();
	}
	return std::format("Spawned {}", entityName);
}

std::string difficultyHandleSingleplayer(std::stringstream& stream, Player* player, World* world) {
	int difficulty;
	
	if (!(stream >> difficulty))
		return std::format("Current difficulty: {}", ((WorldSingleplayer*)world)->chunkLoader.difficulty);

	if (difficulty < 0 || difficulty>2)
		return std::format("Invalid difficulty: {}! must be 0,1 or 2", difficulty);

	((WorldSingleplayer*)world)->chunkLoader.difficulty = difficulty;
	StateSettings::instanceObj.updateDifficulty(difficulty);
	StateSettings::instanceObj.save();
	return std::format("Set difficulty to: {}", difficulty);
}

std::string giveHandleSingleplayer(std::stringstream& stream, Player* player, World* world) {
	std::string first;
	std::string second;
	std::string third;

	uint32_t count = 1;
	if (!(stream >> first)) {
		return "Usage: /give [entity] item [count=1]";
	}
	if (!(stream >> second)) {
		std::replace(first.begin(), first.end(), '_', ' ');
		spawnEntityItem(first, count, player->pos, world);
		return std::format("Given item: {} x1", first);
	}
	if (stream >> third) {
		std::replace(second.begin(), second.end(), '_', ' ');
		try { count = std::stoi(third); }
		catch (std::invalid_argument e) { return "Give count must be a number!"; }
		std::vector<Entity*> entities = getEntities(first, player, world);
		for (auto entity : entities)
			spawnEntityItem(second, count, entity->getPos(), world);
		return std::format("Given item: {} x{} to {} entities", second, count, entities.size());
	}
	try { count = std::stoi(second); }
	catch (std::invalid_argument e) { // two arguments, second is not a number, use [entity item] variant
		std::replace(second.begin(), second.end(), '_', ' ');
		std::vector<Entity*> entities = getEntities(first, player, world);
		for (auto entity : entities)
			spawnEntityItem(second, count, entity->getPos(), world);
		return std::format("Given item: {} x{} to {} entities", second, count, entities.size());
	}
	// two arguments, second is a number, use [item count] variant
	std::replace(first.begin(), first.end(), '_', ' ');
	spawnEntityItem(first, count, player->pos, world);
	return std::format("Given item: {} x{}", first, count);
	
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

std::string rotateHandle(std::stringstream& stream, Player*, World* world) {
	std::string modeRaw; stream >> modeRaw;
	// validate mode
	std::string m = modeRaw;
	bool is180 = false;
	if (m.size() > 2 && m.substr(m.size() - 3) == "180") {
		is180 = true;
		m.erase(m.size() - 3);
	}
	for (char& c : m) c = char(std::toupper(c));
	static constexpr const char* canon[] = { "XY","XZ","XW","YZ","YW","ZW" };
	if (std::find(std::begin(canon), std::end(canon), m) == std::end(canon)) {
		throw ConstraintException(std::format(
			"Unknown rotate plane: '{}'.", modeRaw));
	}

	glm::ivec4 start, end;
	stream >> start.x >> start.y >> start.z >> start.w
		>> end.x >> end.y >> end.z >> end.w;

	int ia = (m[0] == 'X' ? 0 : m[0] == 'Y' ? 1 : m[0] == 'Z' ? 2 : 3);
	int ib = (m[1] == 'X' ? 0 : m[1] == 'Y' ? 1 : m[1] == 'Z' ? 2 : 3);
	int lenA = std::abs(end[ia] - start[ia]);
	int lenB = std::abs(end[ib] - start[ib]);
	if (lenA != lenB) {
		throw ConstraintException(std::format(
			"Rotation area must be square in plane {}.\n"
			"Rotating non-square areas is unsafe and can destroy blocks around it.\n"
			"Use /rotateFree to rotate area of any size.", m));
	}

	auto count = rotateArea(modeRaw, start, end, world);
	return std::format("Rotated {} blocks ({})", count, modeRaw);
}

std::string rotateFreeHandle(std::stringstream& stream, Player*, World* world) {
	std::string modeRaw; stream >> modeRaw;
	// validate mode (allow reversed but not unknown)
	std::string m = modeRaw;
	if (m.size() > 2 && m.substr(m.size() - 3) == "180")
		m.erase(m.size() - 3);
	for (char& c : m) c = char(std::toupper(c));
	static constexpr const char* canon[] = { "XY","XZ","XW","YZ","YW","ZW" };
	std::string ab = (std::find(std::begin(canon), std::end(canon), m) != std::end(canon)
		? m : std::string{ m[1],m[0] });
	if (std::find(std::begin(canon), std::end(canon), ab) == std::end(canon)) {
		throw ConstraintException(std::format(
			"Unknown rotate plane: '{}'.", modeRaw));
	}

	glm::ivec4 start, end;
	stream >> start.x >> start.y >> start.z >> start.w
		>> end.x >> end.y >> end.z >> end.w;

	auto count = rotateArea(modeRaw, start, end, world);
	return std::format("Rotated {} blocks ({})", count, modeRaw);
}