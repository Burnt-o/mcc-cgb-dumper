#pragma once
#include "pch.h"




enum class pointer_type { BASE_OFFSET, EXE_OFFSET};
extern void* g_MCC_baseAddress; 

class multilevel_pointer {
private:
	pointer_type mPointerType;
	void* mBaseAddress;
	std::vector<int64_t> mOffsets;
	static inline std::stringstream mLastError{};
	bool dereference_pointer(void* base, std::vector<int64_t> offsets, void** resolvedOut);
	static std::stringstream* SetLastErrorByRef() 
	{
		mLastError.clear();
		return &mLastError;
	}
public:
	multilevel_pointer() = default;

	multilevel_pointer(const std::vector<int64_t>& offsets)
		: mBaseAddress(nullptr), mOffsets(offsets), mPointerType(pointer_type::EXE_OFFSET)
	{
	}

	multilevel_pointer(void* const& baseAddress, const std::vector<int64_t>& offsets)
		: mBaseAddress(baseAddress), mOffsets(offsets), mPointerType(pointer_type::BASE_OFFSET)
	{
	}

	void updateBaseAddress(void* const& baseAddress);

	bool resolve(void** resolvedOut);

	template<typename T>
	bool readData(T* resolvedOut) 
	{
		void* address;
		if (!this->resolve(&address)) return false;

		*resolvedOut = *(T*)address;
		return true;
	}


	static std::string GetLastError()
	{
		return mLastError.str();
	}
	
	// Could add a system for caching the results of resolve, but I don't think we really need the marginal performance improvement
};



