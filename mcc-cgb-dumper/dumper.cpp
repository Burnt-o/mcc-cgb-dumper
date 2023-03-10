#include "pch.h"
#include "dumper.h"
#include "pointer.h"
#include "utilities.h"

using namespace nlohmann; // for json

namespace dumper
{





	namespace // private vars
	{
		pointer p_CGBobject_array({0x03F7FF18 , 0x40, 0x0}); // the beginning of the array of CGB objects
		int CGBobject_stride = 0x1B0; // how large a CGB object is (ie the distance between one element in the above array and the next element)

		// the following pointers will be relative to the current CGBobject that we're iterating on, as set by updateBaseAddress
		std::vector<pointer*> CGBmembers;
		pointer p_CGBmember_isValid(nullptr, { 0x10 }); //byte equal to 0x10 if valid, 0x0 if not
		pointer p_CGBmember_customGameName(nullptr, { 0x20 });
		pointer p_CGBmember_description(nullptr, { 0x40 });
		pointer p_CGBmember_serverRegion(nullptr, { 0x80 });
		pointer p_CGBmember_playersInGame(nullptr, { 0x120 });
		pointer p_CGBmember_maxPlayers(nullptr, { 0x121 });
		pointer p_CGBmember_pingMilliseconds(nullptr, { 0x124 });
		pointer p_CGBmember_variantIndex(nullptr, { 0x170 });
		pointer p_CGBmember_mapIndex(nullptr, { 0x174 });
		pointer p_CGBmember_variantArray(nullptr, { 0x158, 0x0 });
		pointer p_CGBmember_variantCount(nullptr, { 0x198 });
		pointer p_CGBmember_thisVariantMapsCount(nullptr, { 0x19C });


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

		}

		void update_CGBmemberpointers(void* p_CGBobject)
		{
			// Set all the CGBmember pointers to be relative to the current CGB object
			for (pointer* member : CGBmembers)
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
	std::string parseCGBmember(pointer& p, T type)
	{
		std::optional<void*> pMember = p.resolve();
		return pMember.has_value() ? parseCGBmember(pMember.value(), type) : "NULL";

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
	std::string parseString(pointer& p)
	{
		std::optional<void*> pMember = p.resolve();

		if (!pMember.has_value()) return "NULL";
		return parseString(pMember.value());
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

		void* CGBobject_array = p_CGBobject_array.resolve().has_value() ? p_CGBobject_array.resolve().value() : nullptr;
		if (!CGBobject_array) { PLOG_ERROR << "CGBobjectarray did not resolve"; return; }

		PLOG_VERBOSE << "CGBobject_array" << CGBobject_array;

		
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



			std::optional<void*> pValidFlag = p_CGBmember_isValid.resolve();
			if (!pValidFlag.has_value())
			{ 
				PLOG_ERROR << "pValidFlag was unreadable"; continue; 
			}
			uint32_t validFlag = *(uint32_t*)pValidFlag.value();

			if (validFlag != 0x10) 
			{
				continueFlag = false;
				break;
			}

			std::optional<void*> p_variantIndex =  p_CGBmember_variantIndex.resolve();
			if (!p_variantIndex.has_value()) 
			{ 
				PLOG_ERROR << "p_variantIndex unreadable"; continue; 
			}
			PLOG_VERBOSE << "p_variantIndex: " << p_variantIndex.value();
			int variantIndex = *(int*)p_variantIndex.value();

			if (variantIndex > 0x10 || variantIndex < 0) 
			{ 
				PLOG_ERROR << "variantIndex was invalid value: " << variantIndex; continue; 
			}

			PLOG_VERBOSE << "variantIndex: " << variantIndex;

			std::optional<void*> p_mapIndex = p_CGBmember_mapIndex.resolve();
			if (!p_mapIndex.has_value()) 
			{ 
				PLOG_ERROR << "p_mapIndex unreadable"; continue; 
			}
			PLOG_VERBOSE << "p_mapIndex: " << p_mapIndex.value();
			int mapIndex = *(int*)p_mapIndex.value();
			PLOG_VERBOSE << "mapIndex: " << mapIndex;

			if (mapIndex > 0x10 || mapIndex < 0) 
			{ 
				PLOG_ERROR << "mapIndex was invalid value: " << mapIndex; continue; 
			}

			std::optional<void*> p_variantArray = p_CGBmember_variantArray.resolve();
			if (!p_variantArray.has_value()) 
			{ 
				PLOG_ERROR << "p_variantArray unreadable"; continue; 
			}
			PLOG_VERBOSE << "p_variantArray: " << p_variantArray.value();

			uintptr_t p_currentVariantInfo = (((uintptr_t)p_variantArray.value()) + (variantIndex * 0x128));
			if (IsBadReadPtr((void*)p_currentVariantInfo, 8)) 
			{
				PLOG_ERROR << "p_currentVariantInfo was invalid" << p_currentVariantInfo << ", index: " << variantIndex; continue;
			}
			PLOG_VERBOSE << "p_currentVariantInfo: " << p_currentVariantInfo;



			std::string variant_game = parseString((void*)(p_currentVariantInfo + 0x20));
			std::string variant_name = parseString((void*)(p_currentVariantInfo + 0x40));
			std::string variant_gameType = parseString((void*)(p_currentVariantInfo + 0x60));

			uintptr_t p_currentVariantMapArray = *(uintptr_t*)(p_currentVariantInfo + 0x110);
			if (IsBadReadPtr((void*)p_currentVariantMapArray, 8))
			{
				PLOG_ERROR << "p_currentVariantMapArray was invalid" << p_currentVariantMapArray; continue;
			}
			PLOG_VERBOSE << "p_currentVariantMapArray: " << p_currentVariantMapArray;

			uintptr_t p_currentVariantMapElement = p_currentVariantMapArray + (0xA8 * mapIndex);
			if (IsBadReadPtr((void*)p_currentVariantMapElement, 8))
			{
				PLOG_ERROR << "p_currentVariantMapElement was invalid" << p_currentVariantMapElement; continue;
			}
			PLOG_VERBOSE << "p_currentVariantMapElement: " << p_currentVariantMapElement;

			std::string variant_currentMap = parseString((void*)(p_currentVariantMapElement));



			json thisCG;

			thisCG.push_back(std::pair("Index", std::to_string(CGBobjectindex - 1)));
			thisCG.push_back(std::pair("CustomGameName", parseString(p_CGBmember_customGameName)));
			thisCG.push_back(std::pair("ServerRegion", parseString(p_CGBmember_serverRegion)));
			thisCG.push_back(std::pair("ServerDescription", parseString(p_CGBmember_description)));
			thisCG.push_back(std::pair("PlayersInGame", parseCGBmember(p_CGBmember_playersInGame, (char)NULL)));
			thisCG.push_back(std::pair("MaxPlayers", parseCGBmember(p_CGBmember_maxPlayers, (char)NULL)));
			thisCG.push_back(std::pair("VariantGame", variant_game));
			thisCG.push_back(std::pair("VariantName", variant_name));
			thisCG.push_back(std::pair("VariantGameType", variant_gameType));
			thisCG.push_back(std::pair("CurrentMap", variant_currentMap));
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