#include "Precomp.h"
#include "File.h"

File::File(const std::string& aPath)
	: myPath(aPath)
{
}

File::File(const std::string& aPath, std::vector<char>&& aData)
	: myPath(aPath)
	, myBuffer(std::move(aData))
{
}

bool File::Read()
{
	// opening at the end allows us to know size quickly
	std::ifstream file(myPath, std::ios::ate | std::ios::binary | std::ios::in);
	if (!file.is_open())
	{
		return false;
	}

	size_t size = file.tellg();
	myBuffer.resize(size);
	file.seekg(0);
	file.read(myBuffer.data(), size);
	file.close();
	return true;
}

bool File::Write(const char* aData, size_t aLength)
{
	// Overwrite existing data
	myBuffer.resize(aLength);
	std::memcpy(myBuffer.data(), aData, aLength);

	return Write();
}

bool File::Write() const
{
	std::ofstream file(myPath, std::ios::binary | std::ios::out);
	if (!file.is_open())
	{
		return false;
	}

	file.write(myBuffer.data(), myBuffer.size());
	file.close();
	return true;
}