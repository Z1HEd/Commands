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

constexpr uint32_t errorTextColor = 0x4800FFFF;

std::vector<std::string> previousCommands{};
int currentCommandIndex=-1;

// Sort alias keys by descending length, then lexicographically
struct KeyCmp {
	bool operator()(const std::string& a, const std::string& b) const {
		if (a.size() != b.size())
			return a.size() > b.size();
		return a < b;
	}
};
// Global alias storage
static std::unordered_map<std::string, std::string> aliasMap;
static std::set<std::string, KeyCmp> aliasKeys;


void addAlias(const std::string& key, const std::string& expansion) {
	// Duplicate key?
	if (aliasMap.count(key))
		throw AliasException(std::format("Alias '{}' is already defined.", key));

	aliasMap.emplace(key, expansion);
	aliasKeys.insert(key);
}

void saveAliases() {
	// build the path to aliases.json
	std::string configPath = std::format(
		"{}/aliases.json",
		fdm::getModPath(fdm::modID)
	);

	// serialize aliasMap into JSON
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
		// no file? nothing to load
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
	aliasKeys.clear();

	for (auto it = j.begin(); it != j.end(); ++it) {
		const std::string& key = it.key();
		const std::string& value = it.value().get<std::string>();
		aliasMap[key] = value;
		aliasKeys.insert(key);
	}
}

std::string aliasHandle(const std::unordered_set<std::string>& modifiers,const std::vector<std::string>& parameters, Player* caller, World* world) {

	if (modifiers.contains("remove")) {
		for (auto key : parameters) {
			if (!aliasMap.contains(key))
				throw AliasException(std::format("Tried removing unknown alias: {}", key));
			aliasMap.erase(key);
		}
	}

	if (parameters.size()==0) {
		std::string result = "Known aliases: ";
		for (const auto& [key, val] : aliasMap)
			result += std::format("\n'{}' => '{}'", key, val);
		return result;
	}
	std::string key(parameters[0]);
	if (parameters.size() == 1) {
		return std::format("Alias '{}' => {}", key, aliasMap[key]);
	}
	std::string value(parameters[1]);
	for (int i = 2;i < parameters.size();i++)
		value += " " + parameters[i];
	addAlias(key, value);
	return std::format("Added alias: '{}' => '{}'", key, aliasMap[key]);
}

// Apply all aliases (whole-word matches) to the message
void applyAliases(std::string& msg) {
	for (auto& key : aliasKeys) {
		const std::string& val = aliasMap[key];
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
	saveAliases();
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
		self->chatInput.cursorPos = self->chatInput.text.size() - 1;
	}
	else if (key == GLFW_KEY_DOWN && self->chatOpen) {
		if (!previousCommands.size()) return;
		currentCommandIndex = std::max(--currentCommandIndex, 0);
		self->chatInput.text = previousCommands[previousCommands.size() - 1 - currentCommandIndex];
		self->chatInput.cursorPos = self->chatInput.text.size() - 1;
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

void tokenize(std::stringstream& commandStream,
	std::string& command,
	std::unordered_set<std::string>& modifiers,
	std::vector<std::string>& parameters
) {
	commandStream >> command;
	command.erase(0, 1);

	bool reveivedParameter = false;
	std::string temp;
	while (commandStream >> temp) {
		if (temp[0] == '-' && !reveivedParameter) {
			temp.erase(0, 1);
			modifiers.insert(temp);
			continue;
		}
		reveivedParameter = true;
		parameters.push_back(temp);
	}
}

// Catch commands client-side
$hook(void, StateGame, addChatMessage, Player* player, const stl::string& message, uint32_t color)
{
	if (self->world->getType()!= World::TYPE_SINGLEPLAYER) // If not in singleplayer, let server handle it
		return original(self, player,message,color);

	if (message[0]!='/') 
		return original(self, player, message, color); // If not a command, do nothing

	std::string messageExpanded = message;
	if (message.find("alias ", 0) != 0) // Dont apply aliases to /alias command
		applyAliases(messageExpanded);

	std::stringstream commandStream(messageExpanded);

	std::string command;
	std::unordered_set<std::string> modifiers{};
	std::vector<std::string> parameters{};

	try {
		tokenize(commandStream, command, modifiers, parameters);
	}
	catch (CommandSyntaxException& ex) {
		return original(self, player, ex.what(), errorTextColor);
	}

	if (!commandHandlers.contains(command)) {
		return original(self, player, "Unknown command: " + command, errorTextColor);
	}

	try {
		return original(self, player, commandHandlers.at(command)(modifiers,parameters, player, self->world.get()), color);
	}
	catch (CommandException& ex) {
		return original(self, player, ex.what(), errorTextColor);
	}
}