#pragma once

class UID
{
public:
	static void Init();
	static UID Create();

	void GetString(char* string) const;

private:
	static mt19937 randomGen;
	static size_t thisPCMac;

	// have 2 bytes unoccupied at the start - could reuse as a tag?
	size_t mac;
	uint32_t time;
	uint32_t rndNum;
};