#pragma once

class Tests
{
public:
	static void RunTests();

private:
	static void TestBase64();
	static void TestPool();
	static void TestBinarySerializer();
	static void TestJsonSerializer();
	static void TestUtilMatches();
	static void TestStableVector();
	static void TestStaticVector();
};