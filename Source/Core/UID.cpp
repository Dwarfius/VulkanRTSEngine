#include "Precomp.h"
#include "UID.h"

#ifdef _WIN32
#include <intrin.h>
#include <iphlpapi.h>
#endif

mt19937 UID::ourRandomGen;
size_t UID::ourThisPCMac = 0;
bool UID::ourIsInitialized = false;

void UID::Init()
{
	std::random_device rd;
	ourRandomGen = mt19937(rd());

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
	// TODO: need to check whether it's thread safe or not
	newUID.myRndNum = ourRandomGen();
	return newUID;
}

UID::UID()
	: myMac(0)
	, myTime(0)
	, myRndNum(0)
{
}

void UID::GetString(char* string) const
{
	sprintf(string, "%.16zX%.8X%.8X", myMac, myTime, myRndNum);
	string[32] = '\0';
}