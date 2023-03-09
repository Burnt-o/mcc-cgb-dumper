#include "pch.h"
#include "dumper.h"
#include "pointer.h"

namespace dumper
{
	namespace // private vars
	{
		pointer p_CGBobject_array({0x03F7FF18 , 0x40, 0x0}); // the beginning of the array of CGB objects
		int CGBobject_stride = 0x1B0; // how large a CGB object is (ie the distance between one element in the above array and the next element)

		// the following pointers will be relative to the current CGBobject that we're iterating on, as set by updateBaseAddress
		pointer p_CGBmember_playersInGame(nullptr, { 0x120 });
		pointer p_CGBmember_maxPlayers(nullptr, { 0x121 });
		pointer p_CGBmember_pingMilliseconds(nullptr, { 0x124 });
	}


	void dump()
	{
		PLOG_VERBOSE << "Beginning dump";

		HANDLE mccHandle = GetModuleHandleA(NULL); // null means get the handle for the currently running module
		if (!mccHandle) { PLOG_ERROR << "Couldn't get MCC process handle"; return; }

		g_MCC_baseAddress = mccHandle;
		PLOG_INFO << "MCC found at address: " << mccHandle;



	}
}