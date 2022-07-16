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
	// Expects extension name without the .
	void Draw(std::string_view anExt = {});

	template<class T>
	void DrawFor()
	{
		std::string_view resExt = T::kExtension;
		Draw(resExt.substr(1));
	}

	bool GetPickedFile(File& anAsset);

private:
	std::vector<File> myFiles;
	constexpr static uint8_t kMaxFilterSize = 50;
	char myFilter[kMaxFilterSize]{};
	const File* myPickedFile = nullptr;
	char myLastExt[8]{'*', 0};
};

