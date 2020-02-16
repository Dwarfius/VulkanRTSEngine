#include "Precomp.h"
#include "File.h"

File::File(const std::string& aPath)
	: myPath(aPath)
{
}

bool File::Read()
{
	// opening at the end allows us to know size quickly
	// Note: heard cases where this managed to fail to seek to the end
	// of binary file
	std::ifstream file(myPath, std::ios::ate | std::ios::binary);
	if (!file.is_open())
	{
		return false;
	}

	size_t size = file.tellg();
	myBuffer.resize(size);
	file.seekg(0);
	file.read(&myBuffer[0], size);
	file.close();
	return true;
}