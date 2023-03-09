#pragma once
#include "pch.h"

enum class pointer_type { BASE_OFFSET, EXE_OFFSET};
extern void* g_MCC_baseAddress; 

class pointer {
private:
	pointer_type mPointerType;
	void* mBaseAddress;
	std::vector<int64_t> mOffsets;
	std::optional<void*> dereference_pointer(void* base, std::vector<int64_t> offsets);

public:
	pointer() = default;

	pointer(const std::vector<int64_t>& offsets)
		: mOffsets(offsets), mPointerType(pointer_type::EXE_OFFSET)
	{
	}

	pointer(void* const& baseAddress, const std::vector<int64_t>& offsets)
		: mBaseAddress(baseAddress), mOffsets(offsets), mPointerType(pointer_type::BASE_OFFSET)
	{
	}

	std::optional<void*> resolve();
	void updateBaseAddress(void* const& baseAddress);
	// Could add a system for caching the results of resolve, but I don't think we really need the marginal performance improvement
};


