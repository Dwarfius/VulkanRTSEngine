#pragma once

class Shader;
template<class T> class Handle;

class ShaderCreateDialog
{
public:
	void Draw(bool& aIsOpen);

private:
	void DrawShader();

	static void DrawSaveInput(std::string& aPath);

	uint8_t myShaderType = 0;
	std::string mySource;
	std::string mySavePath;
};