#pragma once

// Note to self, i really should be passing these strings by ref cos the performance is gonna fuckin suck otherwise
class CGBobject_parsed
{
public:
	std::unordered_map<std::string, std::string> mData;

	void AddData(std::string key, std::string value)
	{
		mData.insert_or_assign(key, value);
	}

	std::string GetData(std::string key)
	{
		if (!mData.contains(key)) return "INVALID KEY";
		return mData[key];

	}
};

