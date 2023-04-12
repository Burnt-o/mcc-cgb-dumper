#pragma once
#include "pch.h"
#include "AutoDumper.h"
#include "MultilevelPointer.h"
#include "CustomGameInfoStructs.h"




using namespace nlohmann; // for json




bool AutoDumper::mDumpQueued = false;
safetyhook::InlineHook AutoDumper::mUpdateCustomGameArrayHook;
AutoDumper::MCC_UpdateCustomGameArray AutoDumper::mOrigUpdateCustomGameArray = nullptr;






// Pointers to stuff in the CustomGameInfoArray so we can read all the data we need
	const MultilevelPointer mlp_CustomGameInfoArrayStart{ { 0x03F7FF18 , 0x40, 0x0 } }; // the beginning of the array of CGB objects
	const MultilevelPointer mlp_CustomGameInfoArrayEnd{ { 0x03F7FF18 , 0x40, 0x0 } };



void WriteJsonToFile(json j, std::string_view const &path)
{
	PLOG_VERBOSE << "Writing JSON data to file";
	
	std::ofstream outFile(path.data());
	if (outFile.is_open())
	{
		outFile << j.dump(4);
		outFile.close();
		PLOG_INFO << "JSON written succesfully!";
	}
	else
	{
		PLOG_ERROR << "Failed to open file : " << GetLastError() << std::endl << "at: " << path;
	}
}






void AutoDumper::dump()
{


	PLOG_VERBOSE << "Beginning dump";

	void* p_CustomGameInfoArrayStart;
	if(!mlp_CustomGameInfoArrayStart.resolve(&p_CustomGameInfoArrayStart))
	{ 
		PLOG_ERROR << "mlp_CustomGameInfoArrayStart resolution failed: " << MultilevelPointer::GetLastError(); return;
	}
	PLOG_VERBOSE << "p_CustomGameInfoArray: " << p_CustomGameInfoArrayStart;


	CustomGameInfo* customGameInfoElement = (CustomGameInfo*)p_CustomGameInfoArrayStart;
	int customGameCount = 0;
	json CustomGameInfoArray_Json; // array of CustomGameInfo objects (converted to key-value string pairs) that the following do-while loop will add to

	// There's no count of how many customGameInfoElements there are, or a pointer to the end of the array
	// but we can just check if the gameID.stringLength is 10 - it should always be 10 for valid custom games.
	// Luckily all the valid games are stored at the start and are contiguous
	while (customGameInfoElement->gameID.stringLength == 0x10)
	{
		PLOG_DEBUG << "Processing customGameElement index[" << customGameCount << "]";
		json json_customGameInfoElement;
		customGameCount++;


		// Read off and make json entries of all the data stored directly in the CustomGameInfo
		json_customGameInfoElement["Index"] = customGameCount - 1;
		json_customGameInfoElement["CustomGameName"] = customGameInfoElement->customGameName.copy();
		json_customGameInfoElement["ServerDescription"] = customGameInfoElement->serverDescription.copy();
		json_customGameInfoElement["GameID"] = customGameInfoElement->gameID.copy();
		json_customGameInfoElement["ServerRegionName"] = customGameInfoElement->serverRegionName.copy();
		json_customGameInfoElement["PlayersInGame"] = (int)customGameInfoElement->playersInGame;
		json_customGameInfoElement["MaxPlayers"] = (int)customGameInfoElement->maxPlayers;
		json_customGameInfoElement["PingMilliseconds"] = customGameInfoElement->pingMilliseconds;
		json_customGameInfoElement["IsTeamChangingAllowed"] = customGameInfoElement->isTeamChangingAllowed.copy();
		// Will find these while looping over variants and maps
		json json_currentlyPlayingVariant;
		json json_currentlyPlayingMap;
		// Loop over all the variants
		VariantInfo* variantInfoElement = customGameInfoElement->variantInfoArrayStart;

		json json_variantInfoArray;
		int variantCount = 0;
		int totalMapCount = 0;
		// Luckily the customGameElement has a reference to the end of the array so we can just check for that
		while (variantInfoElement != customGameInfoElement->variantInfoArrayEnd)
		{
			PLOG_DEBUG << "Processing variantInfoElement index[" << variantCount << "]";
			variantCount++;
			json json_variantInfoElement;
			json_variantInfoElement["VariantIndex"] = variantCount - 1;
			json_variantInfoElement["MCCGame"] = variantInfoElement->whichMCCGame.copy();
			json_variantInfoElement["VariantName"] = variantInfoElement->variantName.copy();
			json_variantInfoElement["GameType"] = variantInfoElement->gameType.copy();
			json_variantInfoElement["GameTypeBaseType"] = variantInfoElement->gameTypeBaseType.copy();
			json_variantInfoElement["GameTypeDescription"] = variantInfoElement->gameTypeDescription.copy();

			if (variantCount - 1 == customGameInfoElement->currentlyPlayingVariantIndex)
			{
				json_currentlyPlayingVariant = json_variantInfoElement;
			}

			// Loop over all the maps in this variant
			MapInfo* mapInfoElement = variantInfoElement->mapInfoArrayStart;
			json json_mapInfoArray;
			int mapCount = 0;
			while (mapInfoElement != variantInfoElement->mapInfoArrayEnd)
			{
				PLOG_DEBUG << "Processing mapInfoElement index[" << mapCount << "]";
				mapCount++;
				totalMapCount++;
				json json_mapInfoElement;
				json_mapInfoElement["MapName"] = mapInfoElement->mapName.copy();
				json_mapInfoElement["MapBaseName"] = mapInfoElement->mapBaseName.copy();
				json_mapInfoElement["MapDescription"] = mapInfoElement->mapDescription.copy();
				json_mapInfoArray.emplace_back(json_mapInfoElement);

				if (variantCount - 1 == customGameInfoElement->currentlyPlayingVariantIndex && mapCount - 1 == customGameInfoElement->currentlyPlayingMapIndex)
				{
					json_currentlyPlayingMap = json_mapInfoElement;
				}


				// Advance to the next mapInfoElement
				mapInfoElement++;
			}
			json_variantInfoElement["VariantMapCount"] = mapCount;
			json_variantInfoElement["MapArray"] = json_mapInfoArray;
			json_variantInfoArray.emplace_back(json_variantInfoElement);
			// Advance to next variantInfoElement
			variantInfoElement++;
		}
		json_customGameInfoElement["VariantCount"] = variantCount;
		json_customGameInfoElement["TotalMapCount"] = totalMapCount;
		json_customGameInfoElement["VariantArray"] = json_variantInfoArray;
		json_customGameInfoElement["CurrentlyPlayingVariant"] = json_currentlyPlayingVariant;
		json_customGameInfoElement["CurrentlyPlayingMap"] = json_currentlyPlayingMap;

		CustomGameInfoArray_Json.emplace_back(json_customGameInfoElement);

		// Advance to next customGameInfoElement
		customGameInfoElement++;
	}






	// Add everything to the master json
	json master;
	master.emplace_back(std::pair("CurrentTime", date::format("%F %T", std::chrono::system_clock::now())));
	master.emplace_back(std::pair("CustomGameCount", customGameCount));
	master.emplace_back(std::pair("CustomGameArray", CustomGameInfoArray_Json));


		WriteJsonToFile(master, this->mJsonDumpPath + "CustomGameBrowserData.json");



}
