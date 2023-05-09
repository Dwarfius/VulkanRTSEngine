#pragma once

class File
{
public:
	File(std::string_view aPath);
	File(std::string_view aPath, std::vector<char>&& aData);

	static bool Exists(std::string_view aPath);
	static bool Delete(std::string_view aPath);

	// Blocking until entire file is read
	bool Read();
	bool Write(const char* aData, size_t aLength);
	bool Write() const;

	size_t GetSize() const { return myBuffer.size(); }
	char GetAt(size_t anIndex) const { return myBuffer[anIndex]; }

	const char* GetCBuffer() const { return myBuffer.data(); }
	const std::vector<char>& GetBuffer() const { return myBuffer; }
	std::vector<char>&& ConsumeBuffer() { return std::move(myBuffer); }

	const char* GetPath() const { return myPath.c_str(); }

private:
	std::string myPath;
	std::vector<char> myBuffer; 
};