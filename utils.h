#pragma once
#include <4dm.h>
#include "CommandExceptions.h"

using namespace fdm;
using namespace commandExceptions;

namespace utils {

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

	inline static std::size_t rotateArea(
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

	inline static std::size_t mirrorArea(
		const bool mirrorAxis[4],
		const glm::ivec4& start,
		const glm::ivec4& end,
		World* world
	) {
		auto lexLE = [&](const glm::ivec4& a, const glm::ivec4& b) {
			return std::tie(a.x, a.y, a.z, a.w) <= std::tie(b.x, b.y, b.z, b.w);
			};

		auto mirrorPt = [&](const glm::ivec4& p) {
			return glm::ivec4{
				mirrorAxis[0] ? start.x + end.x - p.x : p.x,
				mirrorAxis[1] ? start.y + end.y - p.y : p.y,
				mirrorAxis[2] ? start.z + end.z - p.z : p.z,
				mirrorAxis[3] ? start.w + end.w - p.w : p.w
			};
			};

		std::size_t count = 0;
		for (int x = start.x; x <= end.x; ++x) {
			for (int y = start.y; y <= end.y; ++y) {
				for (int z = start.z; z <= end.z; ++z) {
					for (int w = start.w; w <= end.w; ++w) {
						glm::ivec4 p{ x,y,z,w };
						glm::ivec4 q = mirrorPt(p);

						if (!lexLE(p, q))
							continue;

						uint8_t idP = world->getBlock(p);
						uint8_t idQ = world->getBlock(q);

						world->setBlockUpdate(p, idQ);
						world->setBlockUpdate(q, idP);

						if (idP == BlockInfo::CHEST || idQ == BlockInfo::CHEST) {
							auto* chestP = (EntityChest*)world->entities.getBlockEntity(p);
							auto* chestQ = (EntityChest*)world->entities.getBlockEntity(q);

							if (chestP) chestP->setPos(q);
							if (chestQ) chestQ->setPos(p);
						}

						++count;
					}
				}
			}
		}

		world->entities.relocateBlockEntities();
		return count;
	}

	inline static void assertArgumentCount(const std::vector<std::string>& args,const std::set<int>& expected){
		if (std::find(expected.begin(), expected.end(), args.size()) == expected.end())
			throw ArgumentCountException(args.size(), expected);
	}
}