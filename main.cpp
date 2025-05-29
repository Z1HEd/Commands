#include <4dm.h>
#include "Handlers.h"
#include "Command.h"
#include "Aliases.h"

using namespace fdm;
using namespace aliases;

// Initialize the DLLMain
initDLL

constexpr uint32_t errorColor = 0xEA3526;
constexpr uint32_t criticalErrorColor = 0xFF0000;

std::vector<std::string> previousCommands{};
int currentCommandIndex=-1;

std::unordered_map<std::string, std::string> aliasMap{};
std::unordered_map<std::string, Command> commands{};

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
	command = command.substr(1);


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

void addCommand(Command command) {
	if (commands.contains(command.name)) 
		throw std::exception("Tried adding an existing command!");
	commands.insert({ command.name, command });
}

void initCommands() {
	addCommand({
		"tp",
		tpHandle,
		"Teleport entities to another entity or position",
		"/tp <entity> - teleport self to entity\n"
		"/tp <entities> <entity> - teleport one or more entities to a singe entity\n"
		"/tp <x> <y> <z> <w> - teleport self to a position\n"
		"/tp <entities> <x> <y> <z> <w> - teleport one or more entities to a position",
		{1, 2, 4, 5}
		});

	addCommand({
		"kill",
		killHandle,
		"Kill one or more entities",
		"/kill <entities> - kills one or more entities",
		{1}
		});

	addCommand({
		"fill",
		fillHandle,
		"Fill an area with a specific block",
		"/fill <x1> <y1> <z1> <w1> <x2> <y2> <z2> <w2> <blockID> - fills the area with a block <blockID>",
		{9}
		});

	addCommand({
		"clone",
		cloneHandle,
		"Clone blocks from one area to another",
		"/clone <x1> <y1> <z1> <w1> <x2> <y2> <z2> <w2> <x3> <y3> <z3> <w3> - clones blocks from a source area (1-2) to a destination area (3)",
		{12}
		});

	addCommand({
		"seed",
		seedHandle,
		"Display the current world seed",
		"/seed - displays the seed",
		{0}
		});

	addCommand({
		"spawn",
		spawnHandle,
		"Spawn an entity at a specified position",
		"/spawn <entityName> <x> <y> <z> <w> - spawn an entity at a specified position",
		{5}
		});

	addCommand({
		"difficulty",
		difficultyHandle,
		"Get or set the game difficulty",
		"/difficulty - displays current difficulty\n"
		"/difficulty {0|1|2} - sets current difficulty",
		{0, 1}
		});

	addCommand({
		"give",
		giveHandle,
		"Give items to yourself or entities",
		"/give <itemName> [<count>] - give yourself an item. <count> is 1 by default\n"
		"/give <entities> <itemName> [<count>] - give one or more entities an item. <count> is 1 by default",
		{1, 2, 3}
		});

	addCommand({
		"rotate",
		rotateHandle,
		"Rotate a block area in a specified plane",
		"/rotate <plane> <x1> <y1> <z1> <w1> <x2> <y2> <z2> <w2> -safely rotates an area 90 or 180 degrees\n"
		"plane is {XY|XZ|XW|YZ|YW|ZW}. {YX|ZX|WX|ZY|WY|WZ} are the same planes, but different direction.\n"
		"Add \"180\" to a plane to rotate 180 degrees (XZ180 etc.)",
		{9}
		});

	addCommand({
		"alias",
		aliasHandle,
		"View, add, remove and view command aliases",
		"/alias — list existing aliases\n"
		"/alias <name> — show expansion for <name>\n"
		"/alias -remove <name> — remove the alias <name>\n"
		"/alias [-overwrite] <name> <expansion…> — add or overwrite an alias",
		{0, 1, 2}
		});

	addCommand({
		"help",
		helpHandle,
		"Show list of commands or usage of a command",
		"/help - show list of existing commands and their descriptions\n"
		"/help </command> - show usage of the </command>",
		{0,1}
		});

	addCommand({
		"clear",
		clearHandle,
		"Clear the chat messages",
		"/clear - clear the chat messages",
		{0}
		});
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

		if (!commands.contains(command)) {
			return original(self, player, "Unknown command: " + command, errorColor);
		}

		try {
			return original(self, player, commands.at(command).handle(modifiers, parameters, player, self->world.get()), color);
		}
		catch (CommandException& ex) {
			return original(self, player, ex.what(), errorColor);
		}
	}
	catch (std::exception e) {
		return original(self, player, std::format("Critical error when executing command: {}",e.what()), criticalErrorColor);
	}
}

// init commands and aliases

$hook(void,StateIntro, init, StateManager& s) {
	original(self, s);
	initCommands();
	loadAliases();
}

$hook(void, WorldServer, WorldServer) {
	original(self);
	initCommands();
	loadAliases();
}