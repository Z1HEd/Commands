#include <4dm.h>

using namespace fdm;

// Initialize the DLLMain
initDLL

using commandHandler = std::string(*)(std::stringstream&, Player*,World*);

std::vector<Entity*> getEntities(std::string entityString, Player* self, World* world) {
	std::vector<Entity*> result = {};
	world->entitiesMutex.lock();
	if (entityString == "@s") {
		result.push_back(world->entities.entities.at(self->EntityPlayerID).get());
	}
	else if (entityString == "@a") {
		for (const auto& pair : world->entities.entities) {
			if (pair.second.get()->isBlockEntity()) continue;
			result.push_back(pair.second.get());
		}
	}
	else if (entityString == "@p") {
		for (const auto& pair : world->entities.entities) {
			Entity* entity = pair.second.get();
			if (entity->isBlockEntity()) continue;
			if (entity->getName()=="Player")
				result.push_back(pair.second.get());
		}
	}
	else if (entityString == "@e") {
		for (const auto& pair : world->entities.entities) {
			Entity* entity = pair.second.get();
			if (entity->isBlockEntity()) continue;
			if (entity->getName() != "Player")
				result.push_back(pair.second.get());
		}
	}
	else {
		for (const auto& pair : world->entities.entities) {
			Entity* entity = pair.second.get();
			if (entity->isBlockEntity()) continue;
			if (entity->getName() == "Player" && ((EntityPlayer*)entity)->displayName == entityString)
				result.push_back(pair.second.get());
		}
	}
	world->entitiesMutex.unlock();
	return result;
}
std::string getEntityName(Entity* entity) {
	if (entity->getName() == "Player") return ((EntityPlayer*)entity)->displayName != "" ?
		((EntityPlayer*)entity)->displayName :
		"Player"; // in singleplayer displayName is ""
	return entity->getName();
}
void setEntityPosition(Entity* entity, glm::vec4 position) {
	if (entity->getName() == "Player") {
		((EntityPlayer*)entity)->player->touchingGround = false;
		((EntityPlayer*)entity)->player->pos=position;
		((EntityPlayer*)entity)->player->hyperplaneUpdateFlag = true;
	}
	else if (entity->getName() == "Spider") {
		((EntitySpider*)entity)->setPos(position);
	}
	else if (entity->getName() == "Butterfly") {
		((EntityButterfly*)entity)->setPos(position);
	}
}
void killEntity(Entity* entity,World* world){
	if (entity->getName() == "Player") {
		((EntityPlayer*)entity)->dead = true;
		((EntityPlayer*)entity)->player->health = 0;
		((EntityPlayer*)entity)->takeDamage(0,world);
	}
	else if (entity->getName() == "Spider") {
		((EntitySpider*)entity)->dead = true;
		((EntitySpider*)entity)->health = 0;
		((EntitySpider*)entity)->takeDamage(0,world);
	}
	else if (entity->getName() == "Butterfly") {
		((EntityButterfly*)entity)->dead = true;
		((EntityButterfly*)entity)->health = 0;
		((EntityButterfly*)entity)->takeDamage(0,world);
	}
}

std::string tpHandler(std::stringstream& stream, Player* player,World* world) {
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

std::string killHandler(std::stringstream& stream, Player* player, World* world) {
	std::string entityString;
	stream >> entityString;
	std::vector<Entity*> entities = getEntities(entityString,player,world);
	for (auto entity : entities) {
		killEntity(entity,world);
	}
	return entities.size() == 1 ? std::format("Killed {}", getEntityName(entities[0])) :
		std::format("Killed {} entities", entities.size());
}

std::string fillHandler(std::stringstream& stream, Player* player, World* world) {
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
						world->addEntityToChunk(chest,world->getChunk(glm::ivec4{ x,y,z,w }));
					}
				}
	return std::format("Filled {} blocks", areaSize);
}

std::string cloneHandler(std::stringstream& stream, Player* player, World* world) {
	constexpr int maxAreaSize = 32 * 32 * 32 * 32;
	glm::ivec4 fromStartPosition;
	glm::ivec4 fromEndPosition;
	glm::ivec4 toStartPosition;

	int blockType;

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

std::unordered_map<std::string, commandHandler> commandHandlers = {
	{"tp",tpHandler},
	{ "kill",killHandler },
	{ "fill",fillHandler },
	{ "clone",cloneHandler }
};

$hook(void,StateGame, keyInput,StateManager& s, int key, int scancode, int action, int mods) {
	if (key != GLFW_KEY_SLASH || action !=GLFW_PRESS || self->chatOpen)
		return original(self,s,key,scancode,action,mods);

	self->chatOpen = true;
	self->resetMouse(s.window);
	self->ui.selectElement(&self->chatInput);
	self->chatInput.text = "/";
	self->chatInput.cursorPos = 1;
}

$hook(void, StateGame, addChatMessage, Player* player, const stl::string& message, uint32_t color)
{
	if (self->world->getType()!= World::TYPE_SINGLEPLAYER) // If not in singleplayer, let server handle it
		return original(self, player,message,color);

	if (message[0]!='/') 
		return original(self, player, message, color); // If not a command, do nothing
	auto string = std::string(message);
	string.erase(0, 1);
	std::stringstream commandStream;
	commandStream << string;
	std::string command;
	commandStream >> command;
	if (!commandHandlers.contains(command)) {
		return original(self, player, "Unknown command: " + command,color);
	}
	original(self,player,commandHandlers.at(command)(commandStream, player, self->world.get()),color);
}