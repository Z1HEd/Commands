
## Commands mod v1.0
- ✅Create empty mod
- ✅Add command catcher
- Add /tp
- Add /spawn
- Add /give
- ✅Add /fill
- Add /time set, /time add, /time setspeed
- ✅Add entity parameter options: by nickname for players,  @a for all entities (including players),@e for all non-player entities, @s for self, @p for all player entities
- ✅Add /kill
- Add /help for a list of commands
- Add /settings list, /settings set
- Add /clear to clear player inventory
- ✅Add /clone to copy area of blocks from one are to other
- ✅Add /rotate
- Add /mirror
- Add /damage to damage entities
- ✅Add /difficulty
- Add /save to save a world
- Add /say to say something in chat
- ✅Add /seed to display world seed

- Add error handling to all commands

- Server commands:
    - Make commands work on a server
    - Add one admin and list of operators to restrict access to certain commands on a server
    - Add /isadmin and /isoperator to check if player has admin or operator status
    - Add /op and /deop to give and take rights from operators
    - Add /kick
    - Add /ban and /unban
    - Add /list to list players on a server
    - Add /whisper to say something to a specific player
    - Add /maxplayers set, /maxplayers show to set and show current maximum amount of players on a server
    - Add /stop, which is equivalent to typing stop in server console.

- Quality of life:
    - Add player name autofill by tab
    - Add automatic parameters - ~ for players position, ^ for player's targeted block
    - Add parameter evaluation (for example, ~+50 for player position+50)

- High level stuff
    -Make up new system for target selectors
    -Add alias system
    -Add aliases for the minecraft-style target selectors
    -Add variables system

- Add a way for other mods to add custom commands