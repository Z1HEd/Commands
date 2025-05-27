#include <4dm.h>
#include "CommandHandlers.h"
using namespace fdm;

// Initialize the DLLMain
initDLL

using commandHandler = std::string(*)(std::stringstream&, Player*,World*);

std::vector<std::string> previousCommands{};
int currentCommandIndex=-1;
std::unordered_map<std::string, commandHandler> commandHandlers = {
	{"tp",tpHandle},
	{ "kill",killHandle },
	{ "fill",fillHandle },
	{ "clone",cloneHandle },
	{"seed", seedHandleSingleplayer},
	{"spawn", spawnHandle},
	{"difficulty", difficultyHandleSingleplayer},
	{"give",giveHandleSingleplayer},
	{"rotate",rotateHandle},
	{"rotateFree",rotateFreeHandle}
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

// Catch commands client-side
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

	try {
		return original(self, player, commandHandlers.at(command)(commandStream, player, self->world.get()), color);
	}
	catch (CommandException& ex) {
		return original(self, player, ex.what(),color);
	}
}