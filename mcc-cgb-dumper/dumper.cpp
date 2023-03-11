#include "pch.h"
#include "dumper.h"
#include "multilevel_pointer.h"
#include "utilities.h"

using namespace nlohmann; // for json

namespace dumper
{





	namespace // private vars
	{
		multilevel_pointer p_CGBobject_array({0x03F7FF18 , 0x40, 0x0}); // the beginning of the array of CGB objects
		int CGBobject_stride = 0x1B0; // how large a CGB object is (ie the distance between one element in the above array and the next element)
		int VariantStride = 0x128; // how large a Variant (info) element is in the VariantArray (which is itself a member of CGBobject)
		int MapStride = 0xA8; // How large a map (info) element is in the map array (which is itself a member of Variant Element)
		// the following pointers will be relative to the current CGBobject that we're iterating on, as set by updateBaseAddress
		std::vector<multilevel_pointer*> CGBmembers;
		multilevel_pointer p_CGBmember_isValid(nullptr, { 0x10 }); //byte equal to 0x10 if valid, 0x0 if not
		multilevel_pointer p_CGBmember_customGameName(nullptr, { 0x20 });
		multilevel_pointer p_CGBmember_description(nullptr, { 0x40 });
		multilevel_pointer p_CGBmember_serverRegion(nullptr, { 0x80 });
		multilevel_pointer p_CGBmember_playersInGame(nullptr, { 0x120 });
		multilevel_pointer p_CGBmember_maxPlayers(nullptr, { 0x121 });
		multilevel_pointer p_CGBmember_pingMilliseconds(nullptr, { 0x124 });
		multilevel_pointer p_CGBmember_variantIndex(nullptr, { 0x170 });
		multilevel_pointer p_CGBmember_mapIndex(nullptr, { 0x174 });
		multilevel_pointer p_CGBmember_variantArray(nullptr, { 0x158, 0x0 });
		multilevel_pointer p_CGBmember_variantCount(nullptr, { 0x1A0 });
		multilevel_pointer p_CGBmember_thisVariantMapsCount(nullptr, { 0x198 });
		multilevel_pointer p_CGBmember_gameID(nullptr, { 0x0 });
		multilevel_pointer p_CGBmember_serverUUID(nullptr, { 0x60 });

		// Relative to variantArray
		multilevel_pointer p_VariantMember_game(nullptr, { 0x20 });
		multilevel_pointer p_VariantMember_name(nullptr, { 0x40 });
		multilevel_pointer p_VariantMember_gameType(nullptr, { 0x60 });
		multilevel_pointer p_VariantMember_mapArray(nullptr, { 0x110, 0x0 });

		// Relative to MapArray
		multilevel_pointer p_MapMember_mapName(nullptr, { 0x0 });


		bool CGBmemberpointers_initialized = false;
		void init_CGBmemberpointers()
		{
			PLOG_VERBOSE << "Initializing CGB memberpointers";
			// All we're doing is adding them all to the vector, this only needs to be done once
			CGBmembers.push_back(&p_CGBmember_isValid);
			CGBmembers.push_back(&p_CGBmember_customGameName);
			CGBmembers.push_back(&p_CGBmember_description);
			CGBmembers.push_back(&p_CGBmember_serverRegion);
			CGBmembers.push_back(&p_CGBmember_playersInGame);
			CGBmembers.push_back(&p_CGBmember_maxPlayers);
			CGBmembers.push_back(&p_CGBmember_pingMilliseconds);
			CGBmembers.push_back(&p_CGBmember_variantIndex);
			CGBmembers.push_back(&p_CGBmember_mapIndex);
			CGBmembers.push_back(&p_CGBmember_variantArray);
			CGBmembers.push_back(&p_CGBmember_variantCount);
			CGBmembers.push_back(&p_CGBmember_thisVariantMapsCount);
			CGBmembers.push_back(&p_CGBmember_gameID);
			CGBmembers.push_back(&p_CGBmember_serverUUID);

		}

		void update_CGBmemberpointers(void* p_CGBobject)
		{
			// Set all the CGBmember pointers to be relative to the current CGB object
			for (multilevel_pointer* member : CGBmembers)
			{
				member->updateBaseAddress(p_CGBobject);
			}
		}
	}


	// Reads data at void* of type T, converts it to a string
	template <typename T>
	std::string parseCGBmember(void* pMember, T type)
	{
		if (IsBadReadPtr(pMember, sizeof(type))) return "NULL";
		T data = *(T*)pMember;
		return std::to_string(data);
	}

	// Overload that resolves a pointer object first then does above
	template <typename T>
	std::string parseCGBmember(multilevel_pointer& p, T type)
	{
		void* pMember; 
		if (!p.resolve(&pMember)) return "NULL";


		return parseCGBmember(pMember, type);

	}

	// String case, needs to be able to handle both MCC's short-string-optimization technique, and regular long strings.
	// Basically the short strings (less than 16 bytes) are stored as char* right there and then.
	// And the long strings instead store a pointer to the char* where they have more space.
	// Note: this function WILL cause a fatal exception if passed an invalid pointer (ie one that isn't actually pointing to a short or long string)
	std::string parseString(void* pMember)
	{

		PLOG_DEBUG << "PARSING STRING AT " << pMember;
		int strLength = *(int*)(((uintptr_t)pMember) + 0x10); // int value of string length is stored at +0x10 from string start
		PLOG_DEBUG << "PARSING STRING, length: " << strLength;
		if (strLength < 0x10) // TODO: double check if it should be "less than 0x10", or "less than or equal to 0x10")
		{
			PLOG_VERBOSE << "short string case";
			//short string case
			return (char*)pMember; // our void* is actually the char*, so we can return that (automatically converts to a new std::string on return)
		}
		else
		{
			PLOG_VERBOSE << "long string case";
			//long string case. follow the pointer to the real char* (ie we're dealing with a char**)
			return *(char**)pMember;
		}
	}

	//overload that reoslves pointer object first then does above
	std::string parseString(multilevel_pointer& p)
	{
		void* pMember;
		if (!p.resolve(&pMember)) return "ERROR";

		return parseString(pMember);
	}

	void dump()
	{
		if (!CGBmemberpointers_initialized) 
		{
			init_CGBmemberpointers(); 
			CGBmemberpointers_initialized = true;
		}

		PLOG_VERBOSE << "Beginning dump";

		HANDLE mccHandle = GetModuleHandleA(NULL); // null means get the handle for the currently running module
		if (!mccHandle) { PLOG_ERROR << "Couldn't get MCC process handle"; return; }

		g_MCC_baseAddress = mccHandle;
		PLOG_INFO << "MCC found at address: " << mccHandle;

		void* CGBobject_array;
		if(!p_CGBobject_array.resolve(&CGBobject_array)) 
		{ PLOG_ERROR << "CGBobjectarray resolution failed: " << multilevel_pointer::GetLastError(); return;
		}

		
		PLOG_VERBOSE << "CGBobject_array: " << CGBobject_array;

		
		int CGBobjectindex = 0;
		bool continueFlag = true;

		json master; // json we're going to add to everything to in the end
		json CGarray = {}; // array of customGameObjects that the following do-while loop will add to

		do
		{
			void* CGBobject = (void*)(((uintptr_t)CGBobject_array) + CGBobjectindex * CGBobject_stride);
			if (IsBadReadPtr(CGBobject, 8)) 
			{ 
				PLOG_ERROR << "Reached unreadable memory while iterating thru CGBobjects, index: " << CGBobjectindex << ", address: " << CGBobject; return; 
			}
			update_CGBmemberpointers(CGBobject);
			PLOG_VERBOSE << "Processing CGBobject with index: " << CGBobjectindex;
			CGBobjectindex++;


			uint32_t validFlag; // Equal to 0x10 when the custom game is valid/alive, 0x0 otherwise
			if (!p_CGBmember_isValid.readData(&validFlag))
			{
				PLOG_ERROR << "p_CGBmember_isValid readData failed: " << multilevel_pointer::GetLastError(); return;
			}

			if (validFlag != 0x10) 
			{
				// This is what ends the do-while loop; we iterate through all the valid CustomGames, 
					// and stop once we reach an invalid one (luckily all the valid ones are grouped together at the start of the array)
				PLOG_INFO << "Found end of CGBobject struct, total games: " << (CGBobjectindex + 1) << ", validFlag value: " << validFlag;
				continueFlag = false; 
				break;
			}


			int variantIndex; // Used to know which element of the variants Array we need to acecss
			if (!p_CGBmember_variantIndex.readData(&variantIndex))
			{
				PLOG_ERROR << "p_CGBmember_variantIndex readData failed: " << multilevel_pointer::GetLastError(); continue;
			}

			if (variantIndex > 0x10 || variantIndex < 0) // can't be more than 6 variants I think, and can't be negative
			{
				PLOG_ERROR << "variantIndex was invalid value: " << variantIndex; continue;
			}
			PLOG_VERBOSE << "variantIndex: " << variantIndex;

			int mapIndex; // The index of the currently playing map, out of all the maps in the current variant. 
			if (!p_CGBmember_mapIndex.readData(&mapIndex))
			{
				PLOG_ERROR << "p_CGBmember_mapIndex readData failed: " << multilevel_pointer::GetLastError(); continue;
			}
			if (mapIndex > 0x10 || mapIndex < 0) // I'm actually not sure what the limit on maps in a variant is. But it can't be negative, that's for sure.
			{
				PLOG_ERROR << "mapIndex was invalid value: " << mapIndex; continue;
			}
			PLOG_VERBOSE << "mapIndex: " << mapIndex;


			void* p_variantArray; // Where the data for each variant is stored sequentially with a stride of 0x128. We'll use variantIndex to access the element of the currently playing variant
			if (!p_CGBmember_variantArray.resolve(&p_variantArray))
			{
				PLOG_ERROR << "p_CGBmember_variantArray resolution failed: " << multilevel_pointer::GetLastError(); continue;
			}
			PLOG_VERBOSE << "p_variantArray: 0x" << p_variantArray;

			// Access the element of variantArray that is currently playing variant using variantIndex
			void* p_variantElement = (void*)((uintptr_t)p_variantArray + (variantIndex * VariantStride));
			if (IsBadReadPtr(p_variantElement, 8))
			{
				PLOG_ERROR << "p_variantElement is bad read ptr: " << p_variantElement << ", index: " << variantIndex << ", variantArray: " << p_variantArray; continue;
			}
			PLOG_VERBOSE << "p_variantElement: 0x" << p_variantElement;


			// Set the p_variantElement as the base address for the pointers that are relative to it
			p_VariantMember_game.updateBaseAddress(p_variantElement);
			p_VariantMember_name.updateBaseAddress(p_variantElement);
			p_VariantMember_gameType.updateBaseAddress(p_variantElement);
			p_VariantMember_mapArray.updateBaseAddress(p_variantElement);
		
			void* p_game;
			if (!p_VariantMember_game.resolve(&p_game))
			{
				PLOG_ERROR << "p_VariantMember_game resolution failed: " << multilevel_pointer::GetLastError(); continue;
			}
			PLOG_VERBOSE << "p_game: 0x" << p_game;
			std::string variant_game = parseString(p_game);


			void* p_name;
			if (!p_VariantMember_name.resolve(&p_name))
			{
				PLOG_ERROR << "p_VariantMember_name resolution failed: " << multilevel_pointer::GetLastError(); continue;
			}
			PLOG_VERBOSE << "p_name: 0x" << p_name;
			std::string variant_name = parseString(p_name);

			void* p_gameType;
			if (!p_VariantMember_gameType.resolve(&p_gameType))
			{
				PLOG_ERROR << "p_VariantMember_gameType resolution failed: " << multilevel_pointer::GetLastError(); continue;
			}
			PLOG_VERBOSE << "p_gameType: 0x" << p_gameType;
			std::string variant_gameType = parseString(p_gameType);

			
			void* p_mapArray;
			if (!p_VariantMember_mapArray.resolve(&p_mapArray))
			{
				PLOG_ERROR << "p_VariantMember_mapArray resolution failed: " << multilevel_pointer::GetLastError(); continue;
			}

			// Access the element of MapArray that is currently playing map using mapIndex
			void* p_mapElement = (void*)((uintptr_t)p_mapArray + (mapIndex * MapStride));
			if (IsBadReadPtr(p_mapElement, 8))
			{
				PLOG_ERROR << "p_mapElement is bad read ptr: " << p_mapElement << ", index: " << mapIndex << ", mapArray: " << p_mapArray; continue;
			}
			
			// Set pMapArray as the base address for the map string
			p_MapMember_mapName.updateBaseAddress(p_mapElement);
			
			void* p_mapString;
			if (!p_MapMember_mapName.resolve(&p_mapString))
			{
				PLOG_ERROR << "p_MapMember_mapName resolution failed: " << multilevel_pointer::GetLastError(); continue;
			}
			std::string currentMap = parseString(p_mapString);


			json thisCG;

			thisCG.push_back(std::pair("Index", std::to_string(CGBobjectindex - 1)));
			thisCG.push_back(std::pair("CustomGameName", parseString(p_CGBmember_customGameName)));
			thisCG.push_back(std::pair("ServerRegion", parseString(p_CGBmember_serverRegion)));
			thisCG.push_back(std::pair("ServerDescription", parseString(p_CGBmember_description)));
			thisCG.push_back(std::pair("GameID", parseString(p_CGBmember_gameID)));
			thisCG.push_back(std::pair("ServerUUID", parseString(p_CGBmember_serverUUID)));
			thisCG.push_back(std::pair("PlayersInGame", parseCGBmember(p_CGBmember_playersInGame, (char)NULL)));
			thisCG.push_back(std::pair("MaxPlayers", parseCGBmember(p_CGBmember_maxPlayers, (char)NULL)));
			thisCG.push_back(std::pair("VariantGame", variant_game));
			thisCG.push_back(std::pair("VariantName", variant_name));
			thisCG.push_back(std::pair("VariantGameType", variant_gameType));
			thisCG.push_back(std::pair("CurrentMap", currentMap));
			thisCG.push_back(std::pair("MapCount", parseCGBmember(p_CGBmember_thisVariantMapsCount, (char)NULL)));
			thisCG.push_back(std::pair("VariantCount", parseCGBmember(p_CGBmember_variantCount, (char)NULL)));
			thisCG.push_back(std::pair("PingMilliseconds", parseCGBmember(p_CGBmember_pingMilliseconds, (uint32_t)NULL)));


			CGarray.push_back(thisCG);


		} while (continueFlag);

		// Add everything to the master json

		master.push_back(std::pair("CurrentTime", date::format("%F %T", std::chrono::system_clock::now())));
		master.push_back(std::pair("CustomGameCount", CGBobjectindex - 1));
		master.push_back(std::pair("CustomGameArray", CGarray));

		PLOG_VERBOSE << "Finished getting CGB data, now writing to file";
		//std::cout << master.dump(4);

		std::ofstream outFile("CustomGameBrowserData.json");
		if (outFile.is_open())
		{
			outFile << master.dump(4);
			outFile.close();
			PLOG_INFO << "Dump finished successfully!";
		}
		else
		{
			PLOG_ERROR << "Failed to open file : " << GetLastError();
		}


	}
}