#include <4dm.h>
#include "CommandHandlers.h"
using namespace fdm;

// Initialize the DLLMain
initDLL

using commandHandler = std::string(*)(std::stringstream&, Player*,World*);

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
	if (key != GLFW_KEY_SLASH || action !=GLFW_PRESS || self->chatOpen || self->player.inventoryManager.isOpen())
		return original(self,s,key,scancode,action,mods);

	self->chatOpen = true;
	self->resetMouse(s.window);
	self->ui.selectElement(&self->chatInput);
	self->chatInput.text = "/";
	self->chatInput.cursorPos = 1;
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