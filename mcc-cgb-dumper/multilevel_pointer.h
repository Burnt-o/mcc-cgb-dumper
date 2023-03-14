#pragma once
#include "pch.h"




enum class pointer_type { BASE_OFFSET, EXE_OFFSET};


class multilevel_pointer {
private:
	static void* mEXEAddress;
	static inline std::stringstream mLastError{};
	static std::stringstream* SetLastErrorByRef()
	{
		mLastError.clear();
		mLastError.str("");
		return &mLastError;
	}

	void* mBaseAddress;
	std::vector<int64_t> mOffsets;
	pointer_type mPointerType;


	bool dereference_pointer(void* base, std::vector<int64_t> offsets, void** resolvedOut);

public:


	explicit multilevel_pointer(const std::vector<int64_t>& offsets)
		: mBaseAddress(nullptr), mOffsets(offsets), mPointerType(pointer_type::EXE_OFFSET)
	{
	}

	explicit multilevel_pointer(void* const& baseAddress, const std::vector<int64_t>& offsets)
		: mBaseAddress(baseAddress), mOffsets(offsets), mPointerType(pointer_type::BASE_OFFSET)
	{
	}

	void updateBaseAddress(void* const& baseAddress);

	bool resolve(void** resolvedOut);


	bool readString(std::string& resolvedOut);

	template<typename T>
	bool readData(T* resolvedOut)
	{
		if (typeid(T) == typeid(std::string))
		{
			return readString(*(std::string*)resolvedOut);
		}

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



