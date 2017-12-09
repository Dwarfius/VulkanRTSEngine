#pragma once

class UID
{
public:
	static void Init();
	static UID Create();

	UID();

	// pass in char[33] array
	void GetString(char* string) const;

	bool operator==(const UID& other) const
	{
		return mac == other.mac && time == other.time && rndNum == other.rndNum;
	}

private:
	static mt19937 randomGen;
	static size_t thisPCMac;

	friend struct std::hash<UID>;

	// have 2 bytes unoccupied at the start - could reuse as a tag?
	size_t mac;
	uint32_t time;
	uint32_t rndNum;
};

namespace std
{
	template<>
	struct hash<UID>
	{
		size_t operator()(const UID& key) const
		{
			return ((hash<size_t>()(key.mac) ^ 
				(hash<uint32_t>()(key.time) << 1)) >> 1) ^ 
				(hash<uint32_t>()(key.rndNum) << 1);
		}
	};
}