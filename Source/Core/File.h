#pragma once

#include "RefCounted.h"

// A simple wrapper for reading and storing data with refcounting.
// The read from disk is done explicitly calling Read(),
// until then, this represents an empty file
class File : public RefCounted
{
	// TODO: make threadable using OnReadDone callback
public:
	File(const std::string& aPath);

	bool Read();

	size_t GetSize() const { return myBuffer.size(); }
	char GetAt(size_t anIndex) const { return myBuffer[anIndex]; }
	const char* GetCBuffer() const { return myBuffer.c_str(); }
	const std::string& GetBuffer() const { return myBuffer; }
	const char* GetPath() const { return myPath.c_str(); }

private:
	std::string myPath;
	std::string myBuffer;
};