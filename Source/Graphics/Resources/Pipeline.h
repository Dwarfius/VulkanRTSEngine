#pragma once

#include <Core/Resources/Resource.h>
#include "../Interfaces/IPipeline.h"
#include "../Resources/Shader.h"
#include "../UniformAdapterRegister.h"

// A base class describing a generic pipeline
class Pipeline final : public Resource, public IPipeline
{
public:
	constexpr static StaticString kExtension = ".ppl";

	Pipeline();
	Pipeline(Resource::Id anId, std::string_view aPath);

	Type GetType() const { return myType; }
	void SetType(Type aType) { myType = aType; }

	size_t GetAdapterCount() const override { return myAdapters.size(); }
	const UniformAdapter& GetAdapter(size_t anIndex) const override { return *myAdapters[anIndex]; }
	void AddAdapter(std::string_view anAdapter);

	void AddShader(const std::string& aShader) { myShaders.push_back(aShader); }

	size_t GetShaderCount() const { return myShaders.size(); }
	const std::string& GetShader(size_t anInd) const { return myShaders[anInd]; }

	std::string_view GetTypeName() const override { return "Pipeline"; }

private:
	void Serialize(Serializer& aSerializer) override;

	Type myType;
	std::vector<const UniformAdapter*> myAdapters;
	std::vector<std::string> myShaders;
};