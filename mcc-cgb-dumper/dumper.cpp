#include "pch.h"
#include "dumper.h"
#include "pointer.h"
#include "CGBobject_parsed.h"
namespace dumper
{
	namespace // private vars
	{
		pointer p_CGBobject_array({0x03F7FF18 , 0x40, 0x0}); // the beginning of the array of CGB objects
		int CGBobject_stride = 0x1B0; // how large a CGB object is (ie the distance between one element in the above array and the next element)

		// the following pointers will be relative to the current CGBobject that we're iterating on, as set by updateBaseAddress
		std::vector<pointer*> CGBmembers;
		pointer p_CGBmember_isValid(nullptr, { 0x10 }); //byte equal to 0x10 if valid, 0x0 if not
		pointer p_CGBmember_playersInGame(nullptr, { 0x120 });
		pointer p_CGBmember_maxPlayers(nullptr, { 0x121 });
		pointer p_CGBmember_pingMilliseconds(nullptr, { 0x124 });

		bool CGBmemberpointers_initialized = false;
		void init_CGBmemberpointers()
		{
			PLOG_VERBOSE << "Initializing CGB memberpointers";
			// All we're doing is adding them all to the vector, this only needs to be done once
			CGBmembers.push_back(&p_CGBmember_isValid);
			CGBmembers.push_back(&p_CGBmember_playersInGame);
			CGBmembers.push_back(&p_CGBmember_maxPlayers);
			CGBmembers.push_back(&p_CGBmember_pingMilliseconds);
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


	
	template <typename T>
	std::string parseCGBmember(void* pMember, T type)
	{
		if (IsBadReadPtr(pMember, sizeof(type))) return "NULL";
		T data = *(T*)pMember;
		return std::to_string(data);
	}

	//overloads
	template <typename T>
	std::string parseCGBmember(pointer& p, T type)
	{
		std::optional<void*> pMember = p.resolve();
		return pMember.has_value() ? parseCGBmember(pMember.value(), type) : "NULL";
	}

	template <typename T>
	std::string parseCGBmember(recursive_string_pointer& p, T type)
	{
		std::optional<void*> pMember = p.resolve();
		return pMember.has_value() ? parseCGBmember(pMember.value(), type) : "NULL";
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

		std::vector<CGBobject_parsed*> parsedData;
		do
		{
			void* CGBobject = (void*)(((uintptr_t)CGBobject_array) + CGBobjectindex * CGBobject_stride);
			if (IsBadReadPtr(CGBobject, 8)) { PLOG_ERROR << "Reached unreadable memory while iterating thru CGBobjects, index: " << CGBobjectindex << ", address: " << CGBobject; return; }
			update_CGBmemberpointers(CGBobject);

			std::optional<void*> pValidFlag = p_CGBmember_isValid.resolve();
			if (!pValidFlag.has_value()) { PLOG_ERROR << "pValidFlag was unreadable"; continue; }
			uint32_t validFlag = *(uint32_t*)pValidFlag.value();

			if (validFlag != 0x10) 
			{
				continueFlag = false;
				break;
			}

			CGBobject_parsed* parse = new CGBobject_parsed(); // allocate to heap
			parse->AddData("Index", std::to_string(CGBobjectindex));
			parse->AddData("PlayersInGame", parseCGBmember(p_CGBmember_playersInGame, (char)NULL));
			parse->AddData("MaxPlayers", parseCGBmember(p_CGBmember_maxPlayers, (char)NULL));
			parse->AddData("PingMilliseconds", parseCGBmember(p_CGBmember_pingMilliseconds, (uint32_t)NULL));
			//TODO: the rest of the members
			//TODO: test if recursive string thing works
			

			parsedData.push_back(parse);

			CGBobjectindex++;

		} while (continueFlag);

		PLOG_VERBOSE << "Finished getting CGB data, now parsing to XML or JSON idk";
		// note to self: add a field for the current time that the xml/json file was generated
		for (CGBobject_parsed* CGdata : parsedData)
		{
			// TODO: parse all this crap to xml or json instead of just logging it to the console 

			PLOG_VERBOSE << "Index: " << CGdata->GetData("Index");
			PLOG_VERBOSE << "Current players: " << CGdata->GetData("PlayersInGame");
			PLOG_VERBOSE << "MaxPlayers: " << CGdata->GetData("MaxPlayers");
			PLOG_VERBOSE << "Ping: " << CGdata->GetData("PingMilliseconds");
		}

		PLOG_INFO << "Dump finished successfully!";
		return;
	}
}