#include <4dm.h>
#include "CommandHandlers.h"
#include <fstream>
using namespace fdm;

// Initialize the DLLMain
initDLL

using commandHandler = std::string(*)(
	const std::unordered_set<std::string>&,
	const std::vector<std::string>&,
	Player*,
	World*);

constexpr uint32_t errorColor = 0xEA3526;
constexpr uint32_t criticalErrorColor = 0xFF0000;

std::vector<std::string> previousCommands{};
int currentCommandIndex=-1;

// Global alias storage
static std::unordered_map<std::string, std::string> aliasMap;

void addAlias(const std::string& key, const std::string& expansion) {
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

void saveAliases() {
	std::string configPath = std::format(
		"{}/aliases.json",
		fdm::getModPath(fdm::modID)
	);

	nlohmann::json j;
	for (auto const& [key, value] : aliasMap) {
		j[key] = value;
	}

	// write out nicely formatted JSON
	std::ofstream ofs(configPath);
	if (!ofs) {
		throw std::runtime_error("Failed to open " + configPath + " for writing");
	}
	ofs << j.dump(4) << "\n";
	ofs.close();
}

void loadAliases() {
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

std::string aliasHandle(const std::unordered_set<std::string>& modifiers,const std::vector<std::string>& parameters, Player* caller, World* world) {

	if (modifiers.contains("remove")) {
		assertArgumentCount(parameters, { 1 });

		std::string key = parseText(parameters[0]);
		if (!aliasMap.contains(key))
			throw AliasException(std::format("Tried removing unknown alias: {}", key));
		
		aliasMap.erase(key);

		return std::format("Alias '{}' has been removed", key);
	}

	if (parameters.size()==0) {
		std::string result = "Known aliases: ";
		for (const auto& [key, val] : aliasMap)
			result += std::format("\n'{}' => '{}'", key, val);
		return result;
	}
	std::string key = parseText(parameters[0]);
	if (parameters.size() == 1) {
		return std::format("Alias '{}' => {}", key, aliasMap[key]);
	}
	std::string value = parameters[1];
	for (int i = 2;i < parameters.size();i++)
		value += " " + parameters[i];
	addAlias(key, value);
	saveAliases();
	return std::format("Added alias: '{}' => '{}'", key, aliasMap[key]);
}

// Apply all aliases (whole-word matches) to the message
void applyAliases(std::string& msg) {
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

static const std::unordered_map<std::string, commandHandler> commandHandlers = {
	{ "tp",        tpHandle },
	{ "kill",      killHandle },
	{ "fill",      fillHandle },
	{ "clone",     cloneHandle },
	{ "seed",      seedHandle },
	{ "spawn",     spawnHandle },
	{ "difficulty",difficultyHandle },
	{ "give",      giveHandle },
	{ "rotate",    rotateHandle },
	{ "alias",     aliasHandle }
};

// Open chat when pressing /
$hook(void,StateGame, keyInput,StateManager& s, int key, int scancode, int action, int mods) {


	if (action != GLFW_PRESS || self->player.inventoryManager.isOpen())
		return original(self,s,key,scancode,action,mods);

	if (key == GLFW_KEY_SLASH && !self->chatOpen) {
		currentCommandIndex = -1;
		self->chatOpen = true;
		self->resetMouse(s.window);
		self->ui.selectElement(&self->chatInput);
		self->chatInput.text = "/";
		self->chatInput.cursorPos = 1;
	}
	else if (key == GLFW_KEY_UP && self->chatOpen) {
		if (!previousCommands.size()) return;
		currentCommandIndex = std::min(++currentCommandIndex, (int)previousCommands.size() - 1);
		self->chatInput.text = previousCommands[previousCommands.size()-1 - currentCommandIndex];
		self->chatInput.cursorPos = self->chatInput.text.size();
	}
	else if (key == GLFW_KEY_DOWN && self->chatOpen) {
		if (!previousCommands.size()) return;
		currentCommandIndex = std::max(--currentCommandIndex, 0);
		self->chatInput.text = previousCommands[previousCommands.size() - 1 - currentCommandIndex];
		self->chatInput.cursorPos = self->chatInput.text.size();
	}
	else if (key == GLFW_KEY_ENTER && self->chatOpen) {
		currentCommandIndex = -1;
		if (self->chatInput.text[0] == '/') {
			auto index = std::find(previousCommands.begin(), previousCommands.end(), self->chatInput.text);
			if (index== previousCommands.end())
				previousCommands.push_back(self->chatInput.text);
			else {
				std::string command = *index;
				previousCommands.erase(index);
				previousCommands.push_back(command);
			}
				
		}
		return original(self, s, key, scancode, action, mods);
	}

	return original(self, s, key, scancode, action, mods);
}

void tokenize(
	std::stringstream& commandStream,
	std::string& command,
	std::unordered_set<std::string>& modifiers,
	std::vector<std::string>& parameters
) {
	// Read command name
	commandStream >> command;
	if (!command.empty() && command.front() == '/') {
		command.erase(0, 1);
	}

	std::string rawToken;

	// Read modifiers
	while (commandStream >> rawToken) {
		if (rawToken.empty() || rawToken.front() != '-')
			break;

		if (rawToken.find('"') != std::string::npos) {
			throw CommandSyntaxException(rawToken, "no quotes are allowed in modifiers");
		}
		modifiers.insert(rawToken.substr(1));
	}

	// Read parameters
	do {
		if (rawToken.empty())
			return;

		// If it starts with a quote, accumulate until the closing quote
		if (rawToken.front() == '"') {
			while (rawToken.back() != '"') {
				std::string next;
				if (!(commandStream >> next)) {
					throw CommandSyntaxException(rawToken, "unterminated quote");
				}
				rawToken += " " + next;
			}
		}

		// Single search for invalid interior or unbalanced quotes
		size_t pos = rawToken.find('"', 1);
		if (pos != std::string::npos && (rawToken.front() != '"' || pos != rawToken.size() - 1)) {
			throw CommandSyntaxException(rawToken, "invalid quote position");
		}

		// Disallow modifiers after parameters
		if (rawToken.front() == '-') {
			throw CommandSyntaxException(rawToken, "modifier after parameter");
		}

		parameters.push_back(rawToken);

	} while (commandStream >> rawToken);
}


// Catch commands client-side
$hook(void, StateGame, addChatMessage, Player* player, const stl::string& message, uint32_t color)
{
	if (self->world->getType()!= World::TYPE_SINGLEPLAYER) // If not in singleplayer, let server handle it
		return original(self, player,message,color);

	try {

		if (message[0] != '/')
			return original(self, player, message, color); // If not a command, do nothing

		std::string messageExpanded = message;
		if (message.find("/alias ", 0) != 0) // Dont apply aliases to /alias command
			applyAliases(messageExpanded);

		std::stringstream commandStream(messageExpanded);

		std::string command;
		std::unordered_set<std::string> modifiers{};
		std::vector<std::string> parameters{};

		tokenize(commandStream, command, modifiers, parameters);

		if (!commandHandlers.contains(command)) {
			return original(self, player, "Unknown command: " + command, errorColor);
		}

		try {
			return original(self, player, commandHandlers.at(command)(modifiers, parameters, player, self->world.get()), color);
		}
		catch (CommandException& ex) {
			return original(self, player, ex.what(), errorColor);
		}
	}
	catch (std::exception e) {
		return original(self, player, std::format("Critical error when executing command: {}",e.what()), criticalErrorColor);
	}
}

$hook(void,StateIntro, init, StateManager& s) {
	original(self, s);
	loadAliases();
}

$hook(void, WorldServer, WorldServer) {
	original(self);
	loadAliases();
}