#pragma once
#include "pch.h"







class MultilevelPointer {
private:
	enum class pointer_type { BASE_OFFSET, EXE_OFFSET };
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


	bool dereference_pointer(void* base, std::vector<int64_t> offsets, void** resolvedOut) const;

public:


	explicit MultilevelPointer(const std::vector<int64_t>& offsets)
		: mBaseAddress(nullptr), mOffsets(offsets), mPointerType(pointer_type::EXE_OFFSET)
	{
	}

	explicit MultilevelPointer(void* const& baseAddress, const std::vector<int64_t>& offsets)
		: mBaseAddress(baseAddress), mOffsets(offsets), mPointerType(pointer_type::BASE_OFFSET)
	{
	}

	void updateBaseAddress(void* const& baseAddress);

	bool resolve(void** resolvedOut) const;


	bool readString(std::string& resolvedOut) const;

	template<typename T>
	bool readData(T* resolvedOut) const
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



