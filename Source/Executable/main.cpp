#include "Precomp.h"
#include <Engine/Game.h>

#include <Core/Utils.h>

void glfwErrorReporter(int code, const char* desc)
{
	printf("GLFW error(%d): %s", code, desc);
}

void RunTests();

int main()
{
	// TODO: get rid of this and all rand() calls
	srand(static_cast<uint32_t>(time(0)));

	RunTests();

	Game* game = new Game(&glfwErrorReporter);
	game->Init();
	while (game->IsRunning())
	{
		game->RunMainThread();
	}
	game->CleanUp();
	delete game;

	return 0;
}

void RunTests()
{
	auto strToBuffer = [](std::string_view aStr) {
		std::vector<char> buffer;
		buffer.resize(aStr.size());
		std::memcpy(buffer.data(), aStr.data(), aStr.size());
		return buffer;
	};

	auto compareBuffers = [](const std::vector<char>& aA, const std::vector<char>& aB) {
		return std::memcmp(aA.data(), aB.data(), aA.size()) == 0;
	};

	// basing on https://en.wikipedia.org/wiki/Base64#Examples
	std::vector<char> initialText = strToBuffer("Man is distinguished, not only by his reason, but by this singular passion from other animals, "
		"which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable "
		"generation of knowledge, exceeds the short vehemence of any carnal pleasure.");
	std::vector<char> encoded = Utils::Base64Encode(initialText);
	std::vector<char> expectedEncoded = strToBuffer("TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlz"
		"IHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2Yg"
		"dGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGlu"
		"dWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRo"
		"ZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=");
	ASSERT(compareBuffers(encoded, expectedEncoded));

	std::vector<char> decoded = Utils::Base64Decode(encoded);
	ASSERT(compareBuffers(decoded, initialText));
}