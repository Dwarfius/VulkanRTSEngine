#include "Precomp.h"
#include "Tests.h"

#include <Graphics/Utils.h>

#include <Core/Profiler.h>
#include <Core/Resources/AssetTracker.h>
#include <Core/Resources/BinarySerializer.h>
#include <Core/Resources/JsonSerializer.h>
#include <Core/Pool.h>
#include <Core/StableVector.h>
#include <Core/StaticVector.h>
#include <Core/Utils.h>

void Tests::RunTests()
{
	TestBase64();
	TestPool();
	TestBinarySerializer();
	TestJsonSerializer();
	TestUtilMatches();
	TestStableVector();
	TestStaticVector();
	TestIntersects();
}

void Tests::TestBase64()
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

void Tests::TestPool()
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
		aSerializer.Serialize("my6", my6);
		aSerializer.Serialize("my7", my7);
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

void Tests::TestBinarySerializer()
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

void Tests::TestJsonSerializer()
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

void Tests::TestUtilMatches()
{
	std::string inputStr = "Some very long string, oh no";
	ASSERT(Utils::Matches(inputStr, "Some"));
	ASSERT(Utils::Matches(inputStr, "*very"));
	ASSERT(Utils::Matches(inputStr, "oh*"));
	ASSERT(Utils::Matches(inputStr, "Some*long*oh"));
	ASSERT(Utils::Matches(inputStr, "**"));
	ASSERT(Utils::Matches(inputStr, "*"));
}

void Tests::TestStableVector()
{
	struct TestType
	{
		std::string myStr;
		uint8_t myNum;
	};

	StableVector<TestType, 2> vec;
	TestType& first = vec.Allocate("Hello", 0);
	TestType& second = vec.Allocate("Hello", 1);
	TestType& third = vec.Allocate("Hello", 2);
	TestType& fourth = vec.Allocate("Hello", 3);
	TestType& fifth = vec.Allocate("Hello", 4);

	{
		uint8_t numbers[]{ 0, 1, 2, 3, 4 };
		int counter = 0;
		vec.ForEach([&](const TestType& aVal) {
			ASSERT(aVal.myStr == "Hello");
			ASSERT(aVal.myNum == numbers[counter]);
			counter++;
		});
		ASSERT(counter == std::extent_v<decltype(numbers)>);

		std::atomic<uint32_t> atomicCounter = 0;
		std::atomic<uint32_t> atomicSum = 0;
		vec.ParallelForEach([&](const TestType& aVal) {
			ASSERT(aVal.myStr == "Hello");
			atomicSum += aVal.myNum;
			atomicCounter++;
		});
		ASSERT(atomicCounter == counter);
		ASSERT(atomicSum == std::accumulate(std::begin(numbers), std::end(numbers), 0));
	}

	vec.Free(third);
	{
		uint8_t numbers[]{ 0, 1, 3, 4 };
		int counter = 0;
		vec.ForEach([&](const TestType& aVal) {
			ASSERT(aVal.myStr == "Hello");
			ASSERT(aVal.myNum == numbers[counter]);
			counter++;
		});
		ASSERT(counter == std::extent_v<decltype(numbers)>);

		std::atomic<uint32_t> atomicCounter = 0;
		std::atomic<uint32_t> atomicSum = 0;
		vec.ParallelForEach([&](const TestType& aVal) {
			ASSERT(aVal.myStr == "Hello");
			atomicSum += aVal.myNum;
			atomicCounter++;
		});
		ASSERT(atomicCounter == counter);
		ASSERT(atomicSum == std::accumulate(std::begin(numbers), std::end(numbers), 0));
	}

	TestType& newThird = vec.Allocate("Hello", 5);
	{
		uint8_t numbers[]{ 0, 1, 5, 3, 4 };
		int counter = 0;
		vec.ForEach([&](const TestType& aVal) {
			ASSERT(aVal.myStr == "Hello");
			ASSERT(aVal.myNum == numbers[counter]);
			counter++;
		});
		ASSERT(counter == std::extent_v<decltype(numbers)>);

		std::atomic<uint32_t> atomicCounter = 0;
		std::atomic<uint32_t> atomicSum = 0;
		vec.ParallelForEach([&](const TestType& aVal) {
			ASSERT(aVal.myStr == "Hello");
			atomicSum += aVal.myNum;
			atomicCounter++;
		});
		ASSERT(atomicCounter == counter);
		ASSERT(atomicSum == std::accumulate(std::begin(numbers), std::end(numbers), 0));
	}

	ASSERT(vec.Contains(&newThird));
	TestType nonExistentType;
	ASSERT(!vec.Contains(&nonExistentType));
}

void Tests::TestStaticVector()
{
	struct TestType
	{
		std::string myStr;
		uint8_t myNum;
	};
	TestType item{ "Hello World", 0 };

	StaticVector<TestType, 5> vec;
	vec.PushBack(item);
	vec.PushBack(TestType{ "Hello", 1 });
	vec.EmplaceBack("World", 2);
	ASSERT(vec.GetSize() == 3);
	ASSERT(vec[1].myStr == "Hello");
	std::string_view strings[]{ "Hello World", "Hello", "World" };
	uint8_t numbers[]{ 0, 1, 2 };
	uint8_t counter = 0;
	for (const TestType& item : vec)
	{
		ASSERT(item.myStr == strings[counter]);
		ASSERT(item.myNum == numbers[counter]);
		counter++;
	}
	ASSERT(counter == vec.GetSize());

	vec.PopBack();
	counter = 0;
	for (const TestType& item : vec)
	{
		ASSERT(item.myStr == strings[counter]);
		ASSERT(item.myNum == numbers[counter]);
		counter++;
	}
	ASSERT(counter == vec.GetSize());
}

void Tests::TestIntersects()
{
	const Utils::AABB aabb{ {0, 0, 0}, {1, 1, 1} };

	// intersecting
	{
		const glm::vec3 a{ 0.1f, 0.1f, 0.1f };
		const glm::vec3 b{ 0.9f, 0.9f, 0.9f };
		const glm::vec3 c{ 0.9f, 0.1f, 0.9f };
		// completelly inside
		ASSERT(Utils::Intersects(a, b, c, aabb));
		ASSERT(Utils::Intersects(b, c, a, aabb));
		ASSERT(Utils::Intersects(c, a, b, aabb));

		// Inside with 0 length edge
		ASSERT(Utils::Intersects(a, a, c, aabb));
		ASSERT(Utils::Intersects(a, c, a, aabb));
		ASSERT(Utils::Intersects(c, a, a, aabb));
		ASSERT(Utils::Intersects(b, b, c, aabb));
		ASSERT(Utils::Intersects(b, c, b, aabb));
		ASSERT(Utils::Intersects(c, b, b, aabb));
		ASSERT(Utils::Intersects(b, c, c, aabb));
		ASSERT(Utils::Intersects(c, c, b, aabb));
		ASSERT(Utils::Intersects(c, b, c, aabb));

		// touching AABB edge, inside
		const glm::vec3 d{ 0.f, 0.f, 0.f };
		const glm::vec3 e{ 0.f, 1.f, 0.f };
		ASSERT(Utils::Intersects(d, e, a, aabb));
		ASSERT(Utils::Intersects(d, e, b, aabb));
		ASSERT(Utils::Intersects(d, e, c, aabb));

		// toucing AABB edge, outside
		const glm::vec3 f{ -1.f, 0.f, 0.f };
		ASSERT(Utils::Intersects(d, e, f, aabb));

		// touching point
		const glm::vec3 g{ -1.f, 1.f, 0.f };
		ASSERT(Utils::Intersects(f, g, a, aabb));

		// cutting the AABB
		// parallel to x
		ASSERT(Utils::Intersects({ 0.5f, -1.f, -1.f }, { 0.5f, 2.f, -1.f }, { 0.5f, 2.f, 2.f }, aabb));
		// parallel to y
		ASSERT(Utils::Intersects({ -1.f, 0.5f, -1.f }, { 2.f, 0.5f, -1.f }, { 2.f, 0.5f, 2.f }, aabb));
		// parallel to z
		ASSERT(Utils::Intersects({ -1.f, -1.f, 0.5f }, { 2.f, -1.f, 0.5f }, { 2.f, 2.f, 0.5f }, aabb));
	}

	// not intersecting
	{
		// left
		ASSERT(!Utils::Intersects({ -1, 0, 0 }, { -1, 1, 0 }, { -1, 1, -1 }, aabb));
		// right
		ASSERT(!Utils::Intersects({ 2, 0, 0 }, { 2, 1, 0 }, { 2, 1, -1 }, aabb));
		// up
		ASSERT(!Utils::Intersects({ 0, 2, 0, }, { 1, 2, 0 }, { 1, 2, 1 }, aabb));
		// down
		ASSERT(!Utils::Intersects({ 0, -1, 0, }, { 1, -1, 0 }, { 1, -1, 1 }, aabb));
		// behind
		ASSERT(!Utils::Intersects({ 0, 0, -1 }, { 0, 1, -1 }, { -1, 1, -1 }, aabb));
		// in front
		ASSERT(!Utils::Intersects({ 0, 0, 2 }, { 0, 1, 2 }, { -1, 1, 2 }, aabb));
	}
}