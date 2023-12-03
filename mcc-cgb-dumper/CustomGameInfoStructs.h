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
	char unknown1[0x68]; // used to be 48
};
static_assert(sizeof(MapInfo) == 0xC8);


struct VariantInfo {
	char unknown1[0x20]; //0x0
	MCCString whichMCCGame; // 0x20
	MCCString variantName; // 0x40
	MCCString gameType; // 0x60
	MCCString gameTypeBaseType; // 0x80
	MCCString unknown; // 0xA0 seems to be a string used in map filtering?
	MCCString unknown4; // 0xC0 new in 3272
	MCCString gameTypeDescription; // 0xE0 
	char unknown2[0x30]; //  0x100 
	MapInfo* mapInfoArrayStart; // 0x130
	MapInfo* mapInfoArrayEnd;
	char unknown3[0x8]; // seems to be a copy of mapInfoArrayEnd
};
static_assert(sizeof(VariantInfo) == 0x148);

struct CustomGameInfo {
	MCCString gameID; // 0x0
	MCCString customGameName; // 0x20
	MCCString serverDescription; // 0x40
	MCCString unknownUUID1; // 0x60
	MCCString serverRegionName; // 0x80
	MCCString isTeamChangingAllowed; // 0xA0
	MCCString unknownUUID2;  // 0xC0
	MCCString unknownUUID3; // 0xE0
	MCCString serverRegion; // 0x100
	char playersInGame; // 0x120
	char maxPlayers;
	char unknown1[0x2];
	uint32_t pingMilliseconds; 
	char unknown2[0x30]; // 0x128 .. used to be 0x30
	VariantInfo* variantInfoArrayStart; // 0x158
	VariantInfo* variantInfoArrayEnd; // 0x160
	char unknown3[0x8]; // 0x168 seems to be a copy of variantInfoArrayEnd
	uint32_t currentlyPlayingVariantIndex; // 0x170
	uint32_t currentlyPlayingMapIndex; //0x174
	char unknown4[0x30]; /// was 38
};
static_assert(sizeof(CustomGameInfo) == 0x1A8);
