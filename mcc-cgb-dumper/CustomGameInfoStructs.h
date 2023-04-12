#pragma once
#include "pch.h"

// This is how MCC does it's short-string-optimization.
// The first 0x10 bytes are a buffer, if the string is less than 0x10 chars
// it's stored right there. Otherwise the first 8 bytes are a pointer to the chars.
struct MCCString {
	union
	{
		char shortString [0x10];
		char* longString;
	};

	size_t stringLength;
	size_t bufferLength;

	
	 std::string copy() const
	{
		if (stringLength < 0x10)
		{
			return std::string(&shortString[0], stringLength);
		}
		else
		{
			if (IsBadReadPtr(longString, stringLength))
			{
				PLOG_ERROR << "Bad string read";
				return "Error";
			}
			return std::string(longString, stringLength);
		}
	}
};


// These structs are the gameData structures we'll be reading,
// the size and layout must be exactly correct
struct MapInfo {
	MCCString mapName;
	MCCString mapBaseName;
	MCCString mapDescription;
	char unknown1[0x48];
};
static_assert(sizeof(MapInfo) == 0xA8);


struct VariantInfo {
	char unknown1[0x20];
	MCCString whichMCCGame;
	MCCString variantName;
	MCCString gameType;
	MCCString gameTypeBaseType;
	MCCString unknown; // seems to be a string used in map filtering?
	MCCString gameTypeDescription;
	char unknown2[0x30];
	MapInfo* mapInfoArrayStart;
	MapInfo* mapInfoArrayEnd;
	char unknown3[0x8]; // seems to be a copy of mapInfoArrayEnd
};
static_assert(sizeof(VariantInfo) == 0x128);

struct CustomGameInfo {
	MCCString gameID;
	MCCString customGameName;
	MCCString serverDescription;
	MCCString unknownUUID1;
	MCCString serverRegionName;
	MCCString isTeamChangingAllowed;
	MCCString unknownUUID2; 
	MCCString unknownUUID3;
	MCCString serverRegion;
	char playersInGame;
	char maxPlayers;
	char unknown1[0x2];
	uint32_t pingMilliseconds;
	char unknown2[0x30];
	VariantInfo* variantInfoArrayStart;
	VariantInfo* variantInfoArrayEnd;
	char unknown3[0x8]; // seems to be a copy of variantInfoArrayEnd
	uint32_t currentlyPlayingVariantIndex;
	uint32_t currentlyPlayingMapIndex;
	char unknown4[0x38];
};
static_assert(sizeof(CustomGameInfo) == 0x1B0);
