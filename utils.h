#pragma once
#include <4dm.h>
#include "Exceptions/CommandExceptions.h"

using namespace fdm;
using namespace commandExceptions;

namespace utils {
	inline static std::vector<Entity*> getEntities(std::string entityString, Player* self, World* world) {
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
				if (entity->getName() == "Player")
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
		else if (entityString == "@i") {
			for (const auto& pair : world->entities.entities) {
				Entity* entity = pair.second.get();
				if (entity->getName() == "Item")
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
		if (result.size() == 0)
			throw EntityNotFoundException(entityString);
		return result;
	}
	
	inline static std::string getEntityName(Entity* entity) {
		if (entity->getName() == "Player") return ((EntityPlayer*)entity)->displayName != "" ?
			((EntityPlayer*)entity)->displayName :
			"Player"; // in singleplayer displayName is ""
		return entity->getName();
	}
	
	inline static void setEntityPosition(Entity* entity, glm::vec4 position, World * world) {
		if (entity->getName() == "Player") {
			((EntityPlayer*)entity)->player->touchingGround = false;
			((EntityPlayer*)entity)->player->pos = position;
			((EntityPlayer*)entity)->player->hyperplaneUpdateFlag = true;
		}
		else if (entity->getName() == "Spider") {
			((EntitySpider*)entity)->setPos(position);
		}
		else if (entity->getName() == "Butterfly") {
			((EntityButterfly*)entity)->setPos(position);
		}
	}
	
	inline static void killEntity(Entity* entity, World* world) {
		if (entity->getName() == "Player") {
			((EntityPlayer*)entity)->player->health = 0;
			((EntityPlayer*)entity)->takeDamage(0, world);
		}
		else if (entity->getName() == "Spider") {
			((EntitySpider*)entity)->health = 0;
			((EntitySpider*)entity)->takeDamage(0, world);
		}
		else if (entity->getName() == "Butterfly") {
			((EntityButterfly*)entity)->health = 0;
			((EntityButterfly*)entity)->takeDamage(0, world);
		}
		entity->dead = true;
	}

	inline static nlohmann::json getDefaultEntityAttributes(const std::string& name) {
		if (name == "Spider") return nlohmann::json{ {"type",rand() % 5} };
		else if (name == "Butterfly") return nlohmann::json{ {"type",rand() % 3} };
		else if (name == "Alidade") return nlohmann::json{
			{"currentBlock", {0, 0, 0, 0}} ,
			{"orientation" , {
				{1, 0, 0, 0, 0},
				{0, 1, 0, 0, 0},
				{0, 0, 1, 0, 0},
				{0, 0, 0, 1, 0},
				{0, 0, 0, 0, 1}
				}
			}
		};
		throw UnknownEntityException(name);
	}

	inline static void spawnEntity(std::string name,glm::vec4 position)
	{
		
		position += 0.0001;

		Chunk* chunk = StateGame::instanceObj.world->getChunkFromCoords(position.x, position.z, position.w);
		if (!chunk) return;

		std::unique_ptr<Entity> entity = Entity::createWithAttributes(name, position, getDefaultEntityAttributes(name));
		StateGame::instanceObj.world->addEntityToChunk(entity, chunk);
	}
	inline static void spawnEntityItem(const std::string& itemName, uint32_t count, const  glm::vec4& position, World* world) {
		std::unique_ptr<Item> item;
		item = Item::create(itemName, count);
		if (!item)
			throw UnknownItemException(itemName);
		std::unique_ptr<Entity> spawnedEntity = EntityItem::createWithItem(
			std::move(item), position, {}
		);
		((EntityItem*)spawnedEntity.get())->combineWithNearby(world);
		Chunk* chunk = world->getChunkFromCoords(position.x, position.z, position.w);
		if (chunk) world->addEntityToChunk(spawnedEntity, chunk);
	}

	// Parsing
	inline static int parseInt(const std::string& s) {
		char* end = nullptr;
		errno = 0;
		long val = std::strtol(s.c_str(), &end, 10);
		if (end == s.c_str() || *end != '\0' || errno == ERANGE) {
			throw ParsingException(s, "int");
		}
		return static_cast<int>(val);
	}

	inline static float parseFloat(const std::string& s) {
		char* end = nullptr;
		errno = 0;
		float val = std::strtof(s.c_str(), &end);
		if (end == s.c_str() || *end != '\0' || errno == ERANGE) {
			throw ParsingException(s, "float");
		}
		return val;
	}
}