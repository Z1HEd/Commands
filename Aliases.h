#pragma once
#include <4dm.h>
#include <fstream>
#include "CommandExceptions.h"
using namespace commandExceptions;

extern std::unordered_map<std::string, std::string> aliasMap;

namespace aliases {

	inline static void saveAliases() {
		std::string configPath = std::format(
			"{}/aliases.json",
			fdm::getModPath(fdm::modID)
		);

		nlohmann::json j;
		for (auto const& [key, value] : aliasMap) {
			j[key] = value;
		}

		std::ofstream ofs(configPath);
		if (!ofs) {
			throw std::runtime_error("Failed to open " + configPath + " for writing");
		}
		ofs << j.dump(4) << "\n";
		ofs.close();
	}

	inline static void addAlias(const std::string& key, const std::string& expansion) {
		if (aliasMap.contains(key))
			throw AliasException(std::format("Alias '{}' is already defined.", key));

		for (auto const& [existingAlias, existingExpansion] : aliasMap) {
			if (expansion.find(existingAlias) != std::string::npos) {
				throw AliasException(std::format(
					"Expansion for '{}' contains existing alias '{}', which would be ambiguous.",
					key, existingAlias
				));
			}
		}
		for (auto const& [existingAlias, existingExpansion] : aliasMap) {
			if (existingExpansion.find(key) != std::string::npos) {
				throw AliasException(std::format(
					"Existing alias '{}' expands to a string containing '{}', which would be ambiguous.",
					existingAlias, key
				));
			}
		}

		aliasMap.emplace(key, expansion);
	}

	inline static void loadAliases() {
		std::string configPath = std::format(
			"{}/aliases.json",
			fdm::getModPath(fdm::modID)
		);

		std::ifstream ifs(configPath);
		if (!ifs) {
			return;
		}

		nlohmann::json j;
		try {
			ifs >> j;
		}
		catch (const std::exception& e) {
			throw std::runtime_error("Failed to parse aliases.json: " + std::string(e.what()));
		}

		aliasMap.clear();
		for (auto it = j.begin(); it != j.end(); ++it)
			aliasMap[it.key()] = it.value().get<std::string>();

	}

	inline static void applyAliases(std::string& msg) {
		for (auto& [key, val] : aliasMap) {
			size_t pos = 0;
			while ((pos = msg.find(key, pos)) != std::string::npos) {
				bool leftOk = (pos == 0 || msg[pos - 1] == ' ');
				bool rightOk = (pos + key.size() == msg.size() || msg[pos + key.size()] == ' ');
				if (leftOk && rightOk) {
					msg.replace(pos, key.size(), val);
					pos += val.size();
				}
				else {
					pos += key.size();
				}
			}
		}
	}
}
