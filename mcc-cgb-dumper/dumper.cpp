#include "pch.h"
#include "dumper.h"
#include "multilevel_pointer.h"
#include "utilities.h"

#include <thread>
#include <chrono>
#include <functional>
#include <cstdio>
#include <atomic>

#include <WinUser.h>

using namespace nlohmann; // for json

namespace dumper
{


	struct Pointers
	{
		multilevel_pointer mlp_CustomGameInfoArray{ { 0x03F7FF18 , 0x40, 0x0 } }; // the beginning of the array of CGB objects
		int CustomGameInfo_Stride = 0x1B0; // how large a CGB object is (ie the distance between one element in the above array and the next element)


				// the following pointers will be relative to the current CGBobject that we're iterating on, as set by updateBaseAddress
		struct CustomGameInfo {
			multilevel_pointer mlp_CustomGameInfo_IsValid{ nullptr, { 0x10 } }; //byte equal to 0x10 if valid, 0x0 if not
			multilevel_pointer mlp_CustomGameInfo_CustomGameName{ nullptr, { 0x20 } };
			multilevel_pointer mlp_CustomGameInfo_Description{ nullptr, { 0x40 } };
			multilevel_pointer mlp_CustomGameInfo_ServerRegion{ nullptr, { 0x80 } };
			multilevel_pointer mlp_CustomGameInfo_PlayersInGame{ nullptr, { 0x120 } };
			multilevel_pointer mlp_CustomGameInfo_MaxPlayers{ nullptr, { 0x121 } };
			multilevel_pointer mlp_CustomGameInfo_PingMilliseconds{ nullptr, { 0x124 } };
			multilevel_pointer mlp_CustomGameInfo_VariantIndex{ nullptr, { 0x170 } };
			multilevel_pointer mlp_CustomGameInfo_MapIndex{ nullptr, { 0x174 } };
			multilevel_pointer mlp_CustomGameInfo_VariantCount{ nullptr, { 0x1A0 } };
			multilevel_pointer mlp_CustomGameInfo_CurrentVariantMapCount{ nullptr, { 0x198 } };
			multilevel_pointer mlp_CustomGameInfo_GameID{ nullptr, { 0x0 } };
			multilevel_pointer mlp_CustomGameInfo_ServerUUID{ nullptr, { 0x60 } };
			multilevel_pointer mlp_CustomGameInfo_VariantInfoArray{ nullptr, { 0x158, 0x0 } };

			std::vector<multilevel_pointer*> all = { &mlp_CustomGameInfo_IsValid , &mlp_CustomGameInfo_CustomGameName, &mlp_CustomGameInfo_Description, &mlp_CustomGameInfo_ServerRegion, &mlp_CustomGameInfo_PlayersInGame, 
				& mlp_CustomGameInfo_MaxPlayers, & mlp_CustomGameInfo_PingMilliseconds, & mlp_CustomGameInfo_VariantIndex , & mlp_CustomGameInfo_MapIndex, 
				& mlp_CustomGameInfo_VariantCount, & mlp_CustomGameInfo_CurrentVariantMapCount, & mlp_CustomGameInfo_GameID, & mlp_CustomGameInfo_ServerUUID, & mlp_CustomGameInfo_VariantInfoArray };
			// Add all the above to a vector so we can easily update the base address with a ranged for-loop
		};
		CustomGameInfo customGameInfo;

		int VariantInfo_Stride = 0x128; // how large a Variant (info) element is in the VariantArray (which is itself a member of CGBobject)
		struct VariantInfo {
			multilevel_pointer mlp_VariantInfo_VariantGame{ nullptr, { 0x20 } };
			multilevel_pointer mlp_VariantInfo_VariantName{ nullptr, { 0x40 } };
			multilevel_pointer mlp_VariantInfo_GameType{ nullptr, { 0x60 } };
			multilevel_pointer mlp_VariantInfo_MapInfoArray{ nullptr, { 0x110, 0x0 } };
			std::vector<multilevel_pointer*> all = { &mlp_VariantInfo_VariantGame , &mlp_VariantInfo_VariantName , &mlp_VariantInfo_GameType , &mlp_VariantInfo_MapInfoArray };
		};
		VariantInfo variantInfo;



		int MapInfo_Stride = 0xA8; // How large a map (info) element is in the map array (which is itself a member of Variant Element)
		struct MapInfo {
			multilevel_pointer mlp_MapInfo_MapName{ nullptr, { 0x0 } };
			std::vector<multilevel_pointer*> all = { &mlp_MapInfo_MapName }; // kinda overkill but at least it's consitent
		};
		MapInfo mapInfo;

		// given a group of MLPs, update their base address
		void GroupUpdateAddresses(std::vector<multilevel_pointer*> const& group, void* pCGBobject)
		{
			for (multilevel_pointer* mlp : group)
			{
				mlp->updateBaseAddress(pCGBobject);
			}
		}

	};


	void WriteJsonToFile(json j)
	{
		PLOG_VERBOSE << "Writing JSON data to file";

		std::ofstream outFile("CustomGameBrowserData.json");
		if (outFile.is_open())
		{
			outFile << j.dump(4);
			outFile.close();
			PLOG_INFO << "JSON written succesfully!";
		}
		else
		{
			PLOG_ERROR << "Failed to open file : " << GetLastError();
		}
	}


	std::string to_string(const uint32_t& val)
	{
		return std::to_string(val);
	}

	std::string to_string(const char& val)
	{
		return std::to_string((int)val);
	}

	std::string to_string(const std::string& val)
	{
		return val;
	}

	template <typename T>
	std::pair<std::string, std::string> ReadDataToJson(multilevel_pointer& mlp_data, const std::string& key)
	{
		T data;
		if (!mlp_data.readData(&data))
		{
			PLOG_ERROR << "error reading data of key: " << key << " : " << multilevel_pointer::GetLastError();
			return std::pair(key, "ERROR");
		}

		std::string dataString = to_string(data);
		return std::pair(key, dataString);

	}



	void* GetElementFromArray(void* p_array, int index, int stride)
	{
		uintptr_t uintptr_array = ((uintptr_t)p_array);
		uintptr_t element = uintptr_array + (index * stride);
		void* p_element = (void*)element;

		if (IsBadReadPtr(p_element, 8)) 
			return nullptr;
		else
			return p_element;
	}

	// Overload that resolves MLP first
	void* GetElementFromArray(multilevel_pointer& mlp_array, multilevel_pointer& mlp_index, int stride)
	{
		void* p_array;
		int index;
		if (!mlp_array.resolve(&p_array)) return nullptr;
		if (!mlp_index.readData(&index)) return nullptr;
		return GetElementFromArray(p_array, index, stride);
	}



	typedef LRESULT(*SendMessageA)(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
	SendMessageA Original_SendMessageA;


	
	const float RefreshXfraction = (440.f / 1220.f);
	const float RefreshYfraction = (690.f / 772.f);

	void ForceCustomGameBrowserRefresh()
	{
		// Get the handle to the MCC window
		HWND handle = FindWindowA(NULL, "Halo: The Master Chief Collection  "); // Two spaces at the end of the string, for some reason
		PLOG_VERBOSE << "MCC Window Handle: " << handle;
		

		if (handle == NULL)
		{
			PLOG_ERROR << "Failed to force a refresh, couldn't get handle to MCC window";
			return;
		}

		// Get the dimensions of the MCC window - we can use these to calculate where the refresh button is located
		RECT windowDimensions;
		GetWindowRect(handle, &windowDimensions);

		float windowWidth = windowDimensions.right - windowDimensions.left;
		float windowHeight = windowDimensions.bottom - windowDimensions.top;

		PLOG_VERBOSE << "windowDimensions: " << std::endl
			<< "width: " << windowWidth << std::endl
			<< "height: " << windowHeight << std::endl;

		WORD refreshX = static_cast<WORD>(windowWidth * RefreshXfraction);
		WORD refreshY = static_cast<WORD>(windowHeight * RefreshYfraction);
		PLOG_VERBOSE << "clickPosition: " << refreshX << ", " << refreshY;
		LPARAM clickPosition = MAKELPARAM(refreshX, refreshY);
		
		// Send the fake click message at where we think the refresh button is
		PostMessageA(handle, WM_MOUSEMOVE, (WPARAM)0, clickPosition);
		PostMessageA(handle, WM_LBUTTONDOWN, (WPARAM)0, clickPosition);
		PostMessageA(handle, WM_LBUTTONUP, (WPARAM)0, clickPosition);



	}


	void dump()
	{
		ForceCustomGameBrowserRefresh();
		static Pointers pointers;

		PLOG_VERBOSE << "Beginning dump";

		void* p_CustomGameInfoArray;
		if(!pointers.mlp_CustomGameInfoArray.resolve(&p_CustomGameInfoArray))
		{ 
			PLOG_ERROR << "mlp_CustomGameInfoArray resolution failed: " << multilevel_pointer::GetLastError(); return;
		}
		PLOG_VERBOSE << "p_CustomGameInfoArray: " << p_CustomGameInfoArray;

		
		int CustomGameInfo_ElementIndex = 0;

		json CustomGameInfoArray_Json = {}; // array of CustomGameInfo objects (converted to key-value string pairs) that the following do-while loop will add to

		do
		{
			auto p_CustomGameInfo = GetElementFromArray(p_CustomGameInfoArray, CustomGameInfo_ElementIndex, pointers.CustomGameInfo_Stride);
			if (IsBadReadPtr(p_CustomGameInfo, 8)) 
			{ 
				PLOG_ERROR << "Reached unreadable memory while iterating thru CustomGameInfoArray, index: " << CustomGameInfo_ElementIndex << ", address: " << p_CustomGameInfo; return; 
			}
			pointers.GroupUpdateAddresses(pointers.customGameInfo.all, p_CustomGameInfo);
			PLOG_VERBOSE << "Processing CustomGameInfo with index: " << CustomGameInfo_ElementIndex;
			CustomGameInfo_ElementIndex++;

			json CustomGameInfo_Json;

			uint32_t validFlag; // Equal to 0x10 when the custom game is valid/alive, 0x0 otherwise
			if (!pointers.customGameInfo.mlp_CustomGameInfo_IsValid.readData(&validFlag))
			{
				PLOG_ERROR << "mlp_CustomGameInfo_IsValid readData failed: " << multilevel_pointer::GetLastError(); return;
			}

			if (validFlag != 0x10) 
			{
				// This is what ends the do-while loop; we iterate through all the valid CustomGames, 
					// and stop once we reach an invalid one (luckily all the valid ones are grouped together at the start of the array)
				PLOG_DEBUG << "Found end of CustomGameInfoArray struct, total custom games: " << (CustomGameInfo_ElementIndex + 1) << ", validFlag value: " << validFlag;

				break;
			}

			// Gets the address of the variant array, then uses the variant index and variant stride to find our variant element
			void* p_variantElement = GetElementFromArray(pointers.customGameInfo.mlp_CustomGameInfo_VariantInfoArray, pointers.customGameInfo.mlp_CustomGameInfo_VariantIndex, pointers.VariantInfo_Stride);
			if (!p_variantElement)
			{
				PLOG_ERROR << "p_variantElement resolution failed: " << multilevel_pointer::GetLastError(); continue;
			}

			// Set the p_variantElement as the base address for the pointers that are relative to it
			pointers.GroupUpdateAddresses(pointers.variantInfo.all, p_variantElement);


			void* p_mapElement = GetElementFromArray(pointers.variantInfo.mlp_VariantInfo_MapInfoArray, pointers.customGameInfo.mlp_CustomGameInfo_MapIndex, pointers.MapInfo_Stride);
			if (!p_mapElement)
			{
				PLOG_ERROR << "p_mapElement resolution failed: " << multilevel_pointer::GetLastError(); continue;
			}

			// Set the p_mapElement as the base address for the pointers that are relative to it
			pointers.GroupUpdateAddresses(pointers.mapInfo.all, p_mapElement);
			

			PLOG_VERBOSE << "Reading off data";
			// Read off and make json entries of all the data we care about
			CustomGameInfo_Json.emplace_back(std::pair("Index", std::to_string(CustomGameInfo_ElementIndex - 1)));

			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.customGameInfo.mlp_CustomGameInfo_CustomGameName, "CustomGameName"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.customGameInfo.mlp_CustomGameInfo_ServerRegion, "ServerRegion"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.customGameInfo.mlp_CustomGameInfo_Description, "ServerDescription"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.customGameInfo.mlp_CustomGameInfo_GameID, "GameID"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.customGameInfo.mlp_CustomGameInfo_ServerUUID, "ServerUUID"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<char>(pointers.customGameInfo.mlp_CustomGameInfo_PlayersInGame, "PlayersInGame"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<char>(pointers.customGameInfo.mlp_CustomGameInfo_MaxPlayers, "MaxPlayers"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.variantInfo.mlp_VariantInfo_VariantGame, "VariantGame"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.variantInfo.mlp_VariantInfo_VariantName, "VariantName"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.variantInfo.mlp_VariantInfo_GameType, "VariantGameType"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<std::string>(pointers.mapInfo.mlp_MapInfo_MapName, "CurrentMap"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<char>(pointers.customGameInfo.mlp_CustomGameInfo_CurrentVariantMapCount, "MapCount"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<char>(pointers.customGameInfo.mlp_CustomGameInfo_VariantCount, "VariantCount"));
			CustomGameInfo_Json.emplace_back(ReadDataToJson<uint32_t>(pointers.customGameInfo.mlp_CustomGameInfo_PingMilliseconds, "PingMilliseconds"));

			CustomGameInfoArray_Json.emplace_back(CustomGameInfo_Json);


		} while (true);

		// Add everything to the master json
		json master;
		master.emplace_back(std::pair("CurrentTime", date::format("%F %T", std::chrono::system_clock::now())));
		master.emplace_back(std::pair("CustomGameCount", CustomGameInfo_ElementIndex - 1));
		master.emplace_back(std::pair("CustomGameArray", CustomGameInfoArray_Json));

		WriteJsonToFile(master);

	}





}