#include "pch.h"
#include "multilevel_pointer.h"


void* g_MCC_baseAddress = nullptr;

bool multilevel_pointer::dereference_pointer(void* base, std::vector<int64_t> offsets, void** resolvedOut)
{
	uintptr_t baseAddress = (uintptr_t)base; //cast to uintptr_t so we can do math to it
	if (offsets.size() > 0)
	{
		for (int i = 0; i < offsets.size(); i++) //skip the first offset since we already handled it
		{
			if (IsBadReadPtr((void*)baseAddress, 8)) // check that it's good to read before we read it
			{
				*SetLastErrorByRef() << "dereferencing failed during looping of offsets" << std::endl
					<< "base: " << base << std::endl
					<< "offsets index at failure" << i << std::endl
					<< "offsets.size() " << offsets.size() << std::endl
					<< "base += offsets[0]: " << (void*)baseAddress << std::endl
					<< "bad read address: " << base << std::endl;
				return false;
			}

			if (i == 0)
			{
				baseAddress += offsets[0];
			}
			else
			{
				baseAddress = (uintptr_t) * (void**)baseAddress; // read the value at baseaddress and assign it to base address
				baseAddress += offsets[i]; // add the offset
			}

		}
	}

	if (IsBadReadPtr((void*)baseAddress, 8)) // check that it's still good to read now that we're done with it
	{
		*SetLastErrorByRef() << "dereferencing failed after looping through offsets" << std::endl
			<< "base: " << base << std::endl
			<< "offsets.size() " << offsets.size() << std::endl
			<< "base += offsets[0]: " << (void*)baseAddress << std::endl
			<< "bad read address: " << base << std::endl;
		return false;
	}

	*resolvedOut = (void*)baseAddress;
	return true;;

}


bool multilevel_pointer::resolve(void** resolvedOut)
{
	switch (this->mPointerType)
	{

	case pointer_type::EXE_OFFSET:
		return dereference_pointer(g_MCC_baseAddress, this->mOffsets, resolvedOut);
		break;

	case pointer_type::BASE_OFFSET:
		return dereference_pointer(this->mBaseAddress, this->mOffsets, resolvedOut);
		break;
	}
}


//template<typename T>
//static bool multilevel_pointer::readData(T* resolvedOut)
//{
//	void* address; 
//	if (!this->resolve(&address)) return false;
//
//	//*resolvedOut = *(T*)address;
//	T* dataPointer = dynamic_cast<T*>(address);
//	if (dataPointer == NULL)
//	{
//		*SetLastErrorByRef() << "Pointer was good but failed casting from void* to " << T << "*";
//			return false;
//	}
//
//	*resolvedOut = *dataPointer;
//		return true;
//}



void multilevel_pointer::updateBaseAddress(void* const& baseAddress)
{
	if (this->mPointerType != pointer_type::BASE_OFFSET) throw;
	this->mBaseAddress = baseAddress;
}

