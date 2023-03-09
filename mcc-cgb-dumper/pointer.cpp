#include "pch.h"
#include "pointer.h"


void* g_MCC_baseAddress = nullptr;

std::optional<void*> pointer::dereference_pointer(void* base, std::vector<int64_t> offsets)
{
	uintptr_t baseAddress = (uintptr_t)base; //cast to uintptr_t so we can do math to it
	if (offsets.size() > 0)
	{
		baseAddress += offsets[0];

		for (int i = 1; i < offsets.size(); i++) //skip the first offset since we already handled it
		{
			if (IsBadReadPtr((void*)baseAddress, 8)) // check that it's good to read before we read it
			{
				PLOG_INFO << "failed dereferencing pointer, bad read @ 0x" << baseAddress;
				return std::nullopt;
			}

			baseAddress = (uintptr_t) * (void**)baseAddress; // read the value at baseaddress and assign it to base address
			baseAddress += offsets[i]; // add the offset
		}
	}

	if (IsBadReadPtr((void*)baseAddress, 8)) // check that it's still good to read
	{
		PLOG_INFO << "failed dereferencing pointer, bad read @ 0x" << baseAddress;
		return std::nullopt;
	}

	return (void*)baseAddress;

}


std::optional<void*> pointer::resolve()
{
	switch (this->mPointerType)
	{

	case pointer_type::EXE_OFFSET:
		return dereference_pointer(g_MCC_baseAddress, this->mOffsets);
		break;

	case pointer_type::BASE_OFFSET:
		return dereference_pointer(this->mBaseAddress, this->mOffsets);
		break;
	}
}


void pointer::updateBaseAddress(void* const& baseAddress)
{
	if (this->mPointerType != pointer_type::BASE_OFFSET) throw;
	this->mBaseAddress = baseAddress;
}



