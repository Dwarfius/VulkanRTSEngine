#include "Precomp.h"
#include "Shader.h"

#include <Core/Resources/Serializer.h>
#include <Core/Resources/AssetTracker.h>

Shader::Shader()
	: myType(IShader::Type::Invalid)
{
}

Shader::Shader(Resource::Id anId, std::string_view aPath)
	: Resource(anId, aPath)
	, myType(IShader::Type::Invalid)
{
}

void Shader::Serialize(Serializer& aSerializer)
{
	aSerializer.Serialize("myType", myType);

	std::string shaderSrcFile = GetPath();
	shaderSrcFile = shaderSrcFile.replace(shaderSrcFile.size() - 3, 3, "txt");
	aSerializer.SerializeExternal(shaderSrcFile, myFileContents, GetId());

	if (myType != Type::Include)
	{
		std::unordered_set<std::string_view> alreadyIncluded;
		myFileContents = PreprocessIncludes(aSerializer, alreadyIncluded);
	}
}

void Shader::Upload(const char* aData, size_t aSize)
{
	myFileContents.resize(aSize);
	std::memcpy(myFileContents.data(), aData, aSize);
}

std::vector<char> Shader::PreprocessIncludes(Serializer& aSerializer, std::unordered_set<std::string_view>& anAlreadyIncluded)
{
	// This assumes we're doing GLSL (which is the only thing we support for now)
	std::string_view text(myFileContents.data(), myFileContents.size());
	size_t index = text.find("#include");
	if (index == std::string_view::npos)
	{
		return myFileContents;
	}

	constexpr uint8_t kInclLength = std::size("#include");
	AssetTracker& assetTracker = aSerializer.GetAssetTracker();
	do
	{
		index += kInclLength;
		size_t endIndex = text.find('\n', index);
		if (endIndex == std::string_view::npos)
		{
			SetErrMsg("Unexpected end of #include line!");
			return myFileContents;
		}

		while (text[index] == ' ')
		{
			index++;
		}
		endIndex--;
		while (text[endIndex] == ' ' || text[endIndex] == '\r')
		{
			endIndex--;
		}

		if (text[index] != '"' || text[endIndex] != '"')
		{
			SetErrMsg("#include - missing \"!");
			return myFileContents;
		}
		index++;

		std::string_view inclName = text.substr(index, endIndex - index);
		inclName = inclName.substr(0, inclName.size() - 4); // drop .txt
		std::string filename(inclName);
		filename.append(kExtension.CStr(), kExtension.GetLength() - 1);
		myDependencies.push_back(assetTracker.GetOrCreate<Shader>(filename));

		index = text.find("#include", endIndex);
	} while (index != std::string_view::npos);

	while (!std::ranges::all_of(myDependencies, [](const Handle<Resource>& aRes) {
		return aRes->GetState() == State::Ready;
	}))
	{
		std::this_thread::yield();
	}

	std::vector<char> newContent;
	uint8_t inclInd = 0;
	index = text.find("#include");
	newContent.reserve(2 * 1024);
	size_t endIndex = 0;
	while (index != std::string_view::npos)
	{
		newContent.append_range(text.substr(endIndex, index - endIndex));
		endIndex = text.find('\n', index);

		Shader* shader = myDependencies[inclInd].Get<Shader>();
		if (shader->GetType() != Type::Include)
		{
			SetErrMsg("Expected include shader to have `Include` type!");
			return myFileContents;
		}

		const auto [iter, inserted] = anAlreadyIncluded.insert(shader->GetPath());
		if (inserted)
		{
			newContent.append_range(shader->PreprocessIncludes(aSerializer, anAlreadyIncluded));
		}

		index = text.find("#include", endIndex);
		inclInd++;
	}
	newContent.append_range(text.substr(endIndex));
	return newContent;
}