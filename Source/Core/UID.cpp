#include "Precomp.h"
#include "UID.h"

#include "Resources/Serializer.h"

#ifdef _WIN32
#include <intrin.h>
#include <iphlpapi.h>
#endif

thread_local std::mt19937 UID::ourRandomGen = []() { 
	// initialize with new thread's id hash
	std::hash<std::thread::id> hasher;
	size_t thisThreadHash = hasher(std::this_thread::get_id());
	// because mersene twister uses 32bit seed, 
	// we need to compress/collapse the hash
	uint32_t seed = static_cast<uint32_t>(thisThreadHash);
	seed ^= static_cast<uint32_t>(thisThreadHash >> 32);
	return std::mt19937(seed);
}();
size_t UID::ourThisPCMac = 0;
bool UID::ourIsInitialized = false;

void UID::Init()
{
	// thanks https://oroboro.com/unique-machine-fingerprint/!
#ifdef _WIN32
	IP_ADAPTER_INFO adapterInfo[32];
	DWORD dwBufLen = sizeof(adapterInfo);

	DWORD dwStatus = GetAdaptersInfo(adapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS)
	{
		return; // no adapters.
	}

	PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
	for (UINT i = 0, end = pAdapterInfo->AddressLength; i < end; i++)
	{
		ourThisPCMac += static_cast<size_t>(pAdapterInfo->Address[i]) << (40 - 8 * i);
	}
#endif

	ourIsInitialized = true;
}

UID UID::Create()
{
	ASSERT_STR(ourIsInitialized, "Tried to create a UID without initializing the system first!");

	UID newUID;
	newUID.myMac = ourThisPCMac;
	newUID.myTime = static_cast<uint32_t>(std::chrono::system_clock::now().time_since_epoch().count());
	newUID.myRndNum = ourRandomGen();
	return newUID;
}

void UID::GetString(char* aString) const
{
	std::snprintf(aString, 32, "%.16zX%.8X%.8X", myMac, myTime, myRndNum);
}

void UID::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myMac", myMac);
	aSerializer.Serialize("myTime", myTime);
	aSerializer.Serialize("myRndNum", myRndNum);
}