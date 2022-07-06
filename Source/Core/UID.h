#pragma once

class Serializer;

class UID
{
public:
	static void Init();
	static UID Create();

	// pass in char[33] array
	void GetString(char* aString) const;

	bool operator==(const UID& other) const
	{
		return myMac == other.myMac 
			&& myTime == other.myTime 
			&& myRndNum == other.myRndNum;
	}

	void Serialize(Serializer& aSerializer);

private:
	static thread_local std::mt19937 ourRandomGen;
	static size_t ourThisPCMac;
	static bool ourIsInitialized;

	friend struct std::hash<UID>;

	// have 2 bytes unoccupied at the start - could reuse as a tag?
	size_t myMac = 0;
	uint32_t myTime = 0;
	uint32_t myRndNum = 0;
};

namespace std
{
	template<>
	struct hash<UID>
	{
		size_t operator()(const UID& key) const
		{
			return ((hash<size_t>()(key.myMac) ^
				(hash<uint32_t>()(key.myTime) << 1)) >> 1) ^
				(hash<uint32_t>()(key.myRndNum) << 1);
		}
	};
}