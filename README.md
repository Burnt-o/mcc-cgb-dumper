# mcc-cgb-dumper
[Halo: The Master Chief Collection](https://store.steampowered.com/app/976730/Halo_The_Master_Chief_Collection/) (aka MCC) is a PC game with a multiplayer custom-game browser (aka CGB) that looks something like [this](https://i.imgur.com/MRimEVe.png). The goal of this tool is to turn that custom-game list into computer-readable json data like [this](https://pastebin.com/92xH62m6) (aka dumping).

This might be useful for say, hooking this data up into a discord bot or some other web-api so users could easily check if there are any custom-games they're interested in playing without having to go boot up MCC themselves. But that is a seperate project.


# How's it work?
We use some simple dll injection to have our code run inside the MCC process, this makes life a lot easier. To force refreshes, we simply call a MCC defined function that does just that.
[SafetyHook](https://github.com/cursey/safetyhook) is used to attach to the MCC function that actually loads in the CGB data - letting it run first obviously then telling our AutoDumper thread to queue a dump.
The dumping itself just involves parsing the Array of CustomGameInfo objects, which to be fair, it took a tad of reverse engineering to figure out the structure there (and particularly how MCC does it's [short-string-optimizations](https://github.com/elliotgoodrich/SSO-23) - I didn't even know that was a thing before this project).
Pointer data is built in at the moment so any MCC updates will break this tool until I get around to updating it.

Includes: [SafetyHook](https://github.com/cursey/safetyhook), [Plog](https://github.com/SergiusTheBest/plog), [json](https://github.com/nlohmann/json), [date.h](https://github.com/HowardHinnant/date)

# How do I use it?
This tool needs MCC to be running with EAC disabled - unfortunately EAC disabled mode also disables the CGB. So first we need to fix this: copy the easyanticheat_xXX.dll's to "%MCCinstalldir%/easyanticheat". Make sure to back them up for when you want to play with EAC on.

Boot up MCC and navigate to the CGB menu. Inject mcc-cgb-dumper with either the provided injector or any dll injector of your choice. A console will open up, if initialization is successful then you should see a list of commands you can enter, such as forcing a refresh or setting up a timer to automatically refresh the CGB every however-many seconds. 
Whenever the CGB is refreshed, the data will be automatically dumped to a json file in the injector's directory, named "CustomGameBrowser.json". You can change this path with set_dump_dir. 
