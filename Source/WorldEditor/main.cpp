#include "Precomp.h"
#include <Engine/Game.h>

#include <Core/Resources/BinarySerializer.h>
#include <Core/Resources/JsonSerializer.h>
#include <Core/Utils.h>

#include "EditorMode.h"

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

	// initialize the game engine
	Game* game = new Game(&glfwErrorReporter);
	game->Init();

	// setup and inject our editor mode task
	EditorMode* editor = new EditorMode(*game);
	constexpr GameTask::Type kEditorUpdateTask = Game::Tasks::Last + 1;
	GameTask editorUpdate(kEditorUpdateTask, [editor, game] {
		const float deltaTime = game->GetLastFrameDeltaTime();
		PhysicsWorld* physWorld = game->GetPhysicsWorld();
		editor->Update(*game, deltaTime, physWorld);
	});
	editorUpdate.AddDependency(Game::Tasks::BeginFrame);
	editorUpdate.AddDependency(Game::Tasks::PhysicsUpdate);
	editorUpdate.AddDependency(Game::Tasks::AnimationUpdate);

	GameTaskManager& taskManager = game->GetTaskManager();
	taskManager.AddTask(editorUpdate);
	taskManager.GetTask(Game::Tasks::UpdateEnd).AddDependency(kEditorUpdateTask);
	taskManager.GetTask(Game::Tasks::RemoveGameObjects).AddDependency(kEditorUpdateTask);

	// start running
	while (game->IsRunning())
	{
		game->RunMainThread();
	}
	
	delete editor;
	game->CleanUp();

	delete game;

	return 0;
}

void TestBase64()
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

void TestPool()
{
	Profiler::ScopedMark profile("Game::TestPool");
	constexpr static auto kSumTest = [](Pool<uint32_t>& aPool, size_t anExpectedSum) {
		int sum = 0;
		aPool.ForEach([&sum](const int& anItem) {
			sum += anItem;
		});
		ASSERT(sum == anExpectedSum);
	};

	Pool<uint32_t> poolOfInts;
	std::vector<Pool<uint32_t>::Ptr> intPtrs;
	// basic test
	intPtrs.emplace_back(std::move(poolOfInts.Allocate(0)));
	ASSERT(*intPtrs[0].Get() == 0);
	ASSERT(poolOfInts.GetSize() == 1);
	intPtrs.clear();
	ASSERT(poolOfInts.GetSize() == 0);

	size_t oldCapacity = poolOfInts.GetCapacity();
	// up to capacity+consistency test 
	size_t expectedSum = 0;
	for (uint32_t i = 0; i < poolOfInts.GetCapacity(); i++)
	{
		intPtrs.emplace_back(std::move(poolOfInts.Allocate(i)));
		expectedSum += i;
	}
	ASSERT(poolOfInts.GetSize() == poolOfInts.GetCapacity());
	ASSERT(poolOfInts.GetCapacity() == oldCapacity);
	kSumTest(poolOfInts, expectedSum);

	// capacity growth test
	uint32_t newElem = static_cast<uint32_t>(poolOfInts.GetCapacity());
	intPtrs.emplace_back(std::move(poolOfInts.Allocate(newElem)));
	expectedSum += newElem;
	ASSERT(poolOfInts.GetCapacity() > oldCapacity);
	kSumTest(poolOfInts, expectedSum);

	// partial clear test
	size_t removedElem = 10;
	intPtrs.erase(intPtrs.begin() + removedElem);
	expectedSum -= removedElem;
	kSumTest(poolOfInts, expectedSum);

	// weak ptr test
	{
		Pool<uint32_t>::WeakPtr weakPtr;
		weakPtr = intPtrs[0];
		ASSERT(*weakPtr.Get() == 0);
	}
	kSumTest(poolOfInts, expectedSum);

	// full clear test
	oldCapacity = poolOfInts.GetCapacity();
	intPtrs.clear();
	ASSERT(poolOfInts.GetSize() == 0);
	ASSERT(poolOfInts.GetCapacity() == oldCapacity);
	kSumTest(poolOfInts, 0);
}

struct Test
{
	struct Inner
	{
		glm::vec3 my1;
		glm::mat4 my2;

		void Serialize(Serializer& aSerializer)
		{
			aSerializer.Serialize("my1", my1);
			aSerializer.Serialize("my2", my2);
		}

		void Match(const Inner& aOther) const
		{
			ASSERT(my1 == aOther.my1);
			ASSERT(my2 == aOther.my2);
		}
	};

	bool my1;
	uint8_t my2;
	int32_t my3;
	uint32_t my4;
	std::string my5;
	std::vector<std::string> my6;
	Inner my7;

	void Serialize(Serializer& aSerializer)
	{
		aSerializer.Serialize("my1", my1);
		aSerializer.Serialize("my2", my2);
		aSerializer.Serialize("my3", my3);
		aSerializer.Serialize("my4", my4);
		aSerializer.Serialize("my5", my5);
		if (Serializer::Scope scope = aSerializer.SerializeArray("my6", my6))
		{
			for (size_t i = 0; i < my6.size(); i++)
			{
				aSerializer.Serialize(i, my6[i]);
			}
		}

		if (Serializer::Scope scope = aSerializer.SerializeObject("my7"))
		{
			my7.Serialize(aSerializer);
		}
	}

	void Match(const Test& aOther) const
	{
		ASSERT(my1 == aOther.my1);
		ASSERT(my2 == aOther.my2);
		ASSERT(my3 == aOther.my3);
		ASSERT(my4 == aOther.my4);
		ASSERT(my5 == aOther.my5);

		ASSERT(my6.size() == aOther.my6.size());
		for (size_t i = 0; i < my6.size(); i++)
		{
			ASSERT(my6[i] == aOther.my6[i]);
		}

		my7.Match(aOther.my7);
	}
};

Test nominalTest{
	true,
	255,
	-10000,
	20000,
	"Hello",
	{ " Binary ", " World!" },
	{ glm::vec3(1, 2, 3), glm::mat4(9) }
};

void TestBinarySerializer()
{
	AssetTracker dummyTracker;
	BinarySerializer writeSerializer(dummyTracker, false);
	nominalTest.Serialize(writeSerializer);
	std::vector<char> buffer;
	writeSerializer.WriteTo(buffer);

	BinarySerializer readSerializer(dummyTracker, true);
	readSerializer.ReadFrom(buffer);
	Test testRead;
	testRead.Serialize(readSerializer);

	testRead.Match(nominalTest);
}

void TestJsonSerializer()
{
	AssetTracker dummyTracker;
	JsonSerializer writeSerializer(dummyTracker, false);
	nominalTest.Serialize(writeSerializer);
	std::vector<char> buffer;
	writeSerializer.WriteTo(buffer);

	JsonSerializer readSerializer(dummyTracker, true);
	readSerializer.ReadFrom(buffer);
	Test testRead;
	testRead.Serialize(readSerializer);

	testRead.Match(nominalTest);
}

void TestUtilMatches()
{
	std::string inputStr = "Some very long string, oh no";
	ASSERT(Utils::Matches(inputStr, "Some"));
	ASSERT(Utils::Matches(inputStr, "*very"));
	ASSERT(Utils::Matches(inputStr, "oh*"));
	ASSERT(Utils::Matches(inputStr, "Some*long*oh"));
	ASSERT(Utils::Matches(inputStr, "**"));
	ASSERT(Utils::Matches(inputStr, "*"));
}

void RunTests()
{
	TestBase64();
	TestPool();
	TestBinarySerializer();
	TestJsonSerializer();
	TestUtilMatches();
}