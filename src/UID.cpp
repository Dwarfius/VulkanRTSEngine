#include "Common.h"
#include "UID.h"

#ifdef _WIN32
#include <intrin.h>
#include <iphlpapi.h>
#endif

mt19937 UID::randomGen = mt19937(std::chrono::system_clock::now().time_since_epoch().count());
size_t UID::thisPCMac = 0;

void UID::Init()
{
	// thanks https://oroboro.com/unique-machine-fingerprint/!
#ifdef _WIN32
	IP_ADAPTER_INFO adapterInfo[32];
	DWORD dwBufLen = sizeof(adapterInfo);

	DWORD dwStatus = GetAdaptersInfo(adapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS)
		return; // no adapters.

	PIP_ADAPTER_INFO pAdapterInfo = adapterInfo;
	for (UINT i = 0, end = pAdapterInfo->AddressLength; i < end; i++)
	{
		thisPCMac += static_cast<size_t>(pAdapterInfo->Address[i]) << (40 - 8 * i);
	}
#endif
}

UID UID::Create()
{
	UID newUID;
	newUID.mac = thisPCMac;
	newUID.time = std::chrono::system_clock::now().time_since_epoch().count();
	newUID.rndNum = randomGen();
	return newUID;
}

void UID::GetString(char* string) const
{
	sprintf(string, "%0.16zX%0.8X%0.8X", mac, time, rndNum);
}