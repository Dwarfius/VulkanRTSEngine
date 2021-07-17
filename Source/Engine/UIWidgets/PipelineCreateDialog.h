#pragma once

#include <Graphics/Descriptor.h>

class Pipeline;
template<class T> class Handle;

class PipelineCreateDialog
{
	// we use our own constant instead of pulling in entire Shader header
	static constexpr uint8_t kShaderTypes = 6;
public:
	void Draw(bool& aIsOpen);
private:
	void DrawPipeline();
	void DrawDescriptor(Descriptor& aDesc);

	static void DrawSaveInput(std::string& aPath);

	uint8_t myPipelineType = 0;
	std::string myShaderPaths[kShaderTypes];
	std::vector<Descriptor> myDescriptors;
	std::string mySavePath;
};