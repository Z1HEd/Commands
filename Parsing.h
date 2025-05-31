#pragma once
#include "tinyexpr-master/tinyexpr.h"
#include <4dm.h>
#include "CommandExceptions.h"
#include "utils.h"

using namespace fdm;
using namespace commandExceptions;

namespace parsing{
	inline static float parseFloat(const std::string& s) {
		int err = 0;
		double result = te_interp(s.c_str(), &err);
		if (err != 0) throw std::runtime_error("Invalid expression: " + s);
		return static_cast<float>(result);
	}

	inline static int parseInt(const std::string& s) {
		float fval = parseFloat(s);
		float rounded = std::round(fval);
		if (std::fabs(fval - rounded) > 1e-6f) throw ParsingException(s, "int");
		if (rounded < static_cast<float>(std::numeric_limits<int>::min()) ||
			rounded > static_cast<float>(std::numeric_limits<int>::max()))
			throw ParsingException(s, "int");
		return static_cast<int>(rounded);
	}

	inline static glm::vec4 parsePosition(
		const std::vector<std::string>& parameters,
		size_t                         startIndex,
		Player* player
	) {
		glm::vec4 pos;
		for (int i = 0; i < 4; ++i) {
			if (startIndex + i >= parameters.size())
				throw ParsingException("missing coordinate at index " + std::to_string(i), "position");
			const std::string& coordToken = parameters[startIndex + i];
			float baseValue = player->currentBlock[i];
			float targetValue = player->targetingBlock
				? player->targetBlock[i]
				: player->reachEndpoint[i];
				std::string expr;
				expr.reserve(coordToken.size() * 3);
				for (char c : coordToken) {
					if (c == '~') expr += std::to_string(baseValue);
					else if (c == '^') expr += std::to_string(targetValue);
					else expr.push_back(c);
				}
				pos[i] = parseFloat(expr);
		}
		return pos;
	}

	inline static std::pair<glm::ivec4, glm::ivec4> parseArea(const std::vector<std::string>& parameters, std::size_t firstIndex, Player* player) {
		glm::ivec4 posA = parsePosition(parameters, firstIndex, player);
		glm::ivec4 posB = parsePosition(parameters, firstIndex + 4, player);


		glm::ivec4 startPos = {
			std::min(posA.x, posB.x),
			std::min(posA.y, posB.y),
			std::min(posA.z, posB.z),
			std::min(posA.w, posB.w)
		};

		glm::ivec4 endPos = {
			std::max(posA.x, posB.x),
			std::max(posA.y, posB.y),
			std::max(posA.z, posB.z),
			std::max(posA.w, posB.w)
		};

		return { startPos, endPos };
	}

	inline static std::string parseText(const std::string& s) {

		if (s.empty()) throw ParsingException(s, "text");

		if (s.front() == '"') {
			return s.substr(1, s.size() - 2);;
		}

		return s;
	}

	inline static std::vector<Entity*> parseEntityList(const std::string& token, Player* self, World* world) {
		std::string ref = token;
		if (ref.size() >= 2 && ref.front() == '"' && ref.back() == '"') {
			ref = ref.substr(1, ref.size() - 2);
		}

		std::vector<Entity*> result;

		std::lock_guard<std::mutex> guard(world->entitiesMutex);

		if (ref == "@s") {
			result.push_back(world->entities.entities.at(self->EntityPlayerID).get());
		}
		else if (ref == "@a") {
			for (const auto& pair : world->entities.entities) {
				if (pair.second->isBlockEntity()) continue;
				result.push_back(pair.second.get());
			}
		}
		else if (ref == "@p") {
			for (const auto& pair : world->entities.entities) {
				Entity* entity = pair.second.get();
				if (entity->isBlockEntity()) continue;
				if (entity->getName() == "Player")
					result.push_back(entity);
			}
		}
		else if (ref == "@e") {
			for (const auto& pair : world->entities.entities) {
				Entity* entity = pair.second.get();
				if (entity->isBlockEntity()) continue;
				if (entity->getName() != "Player")
					result.push_back(entity);
			}
		}
		else if (ref == "@i") {
			for (const auto& pair : world->entities.entities) {
				Entity* entity = pair.second.get();
				if (entity->getName() == "Item")
					result.push_back(entity);
			}
		}
		else {
			for (const auto& pair : world->entities.entities) {
				Entity* entity = pair.second.get();
				if (entity->isBlockEntity()) continue;
				if (entity->getName() == "Player" && ((EntityPlayer*)entity)->displayName == ref)
					result.push_back(entity);
			}
		}

		if (result.empty()) {
			throw EntityNotFoundException(token);
		}

		return result;
	}

	inline static Entity* parseEntity(const std::string& token, Player* self, World* world) {
		std::vector<Entity*> list = parseEntityList(token, self, world);
		if (list.size() != 1) {
			throw MultipleEntitiesException(token, list.size());
		}
		return list[0];
	}
}