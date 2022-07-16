#pragma once

class FileDialog
{
public:
	struct File
	{
		std::string myPath;
		std::string myType;

		auto operator<=>(const File& aOther) const
		{
			if (auto res = myType <=> aOther.myType; res != 0)
			{
				return res;
			}
			return myPath <=> aOther.myPath;
		}
	};

public:
	void Draw();
	bool GetPickedFile(File& anAsset);

private:
	std::vector<File> myFiles;
	constexpr static uint8_t kMaxFilterSize = 50;
	char myFilter[kMaxFilterSize]{};
	const File* myPickedFile = nullptr;
};

