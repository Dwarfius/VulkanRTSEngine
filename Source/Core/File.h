#pragma once

#include "RefCounted.h"

// A simple wrapper for reading and storing data with refcounting.
// The read from disk is done explicitly calling Read(),
// until then, this represents an empty file
class File : public RefCounted
{
public:
	File(const std::string& aPath);

	// Blocking until entire file is read
	bool Read();

	size_t GetSize() const { return myBuffer.size(); }
	char GetAt(size_t anIndex) const { return myBuffer[anIndex]; }

	const char* GetCBuffer() const { return myBuffer.data(); }
	const std::string& GetBuffer() const { return myBuffer; }

	const char* GetPath() const { return myPath.c_str(); }

private:
	std::string myPath;
	// TODO: replace with std::vector<char>, and allow moving from File into
	// std::vector<char> buffer (essentially stealing the internal buffer)
	std::string myBuffer; 
};