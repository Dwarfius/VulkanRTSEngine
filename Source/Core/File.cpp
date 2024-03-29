#include "Precomp.h"
#include "File.h"

#include <filesystem>

File::File(std::string_view aPath)
	: myPath(aPath.data(), aPath.size())
{
}

File::File(std::string_view aPath, std::vector<char>&& aData)
	: myPath(aPath.data(), aPath.size())
	, myBuffer(std::move(aData))
{
}

bool File::Exists(std::string_view aPath)
{
	[[maybe_unused]] std::error_code error;
	bool exists = std::filesystem::exists(aPath, error);
	return exists;
}

bool File::Delete(std::string_view aPath)
{
	[[maybe_unused]] std::error_code error;
	return std::filesystem::remove(aPath, error);
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
	const std::string_view directory = std::string_view{myPath}
		.substr(0, myPath.rfind('/'));
	if (!std::filesystem::exists(directory))
	{
		std::filesystem::create_directories(directory);
	}

	std::ofstream file(myPath, std::ios::binary | std::ios::out);
	if (!file.is_open())
	{
		return false;
	}

	file.write(myBuffer.data(), myBuffer.size());
	file.close();
	return true;
}