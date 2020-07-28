#pragma once

#include <Core/Profiler.h>

class ProfilerUI
{
	constexpr static float kThreadColumnWidth = 80.f;
	constexpr static float kMarkHeight = 30.f;
	using MarksVec = std::vector<Profiler::Mark>;
	using ThreadMarkMap = std::unordered_map<std::thread::id, MarksVec>;
	using HierarchyMap = std::unordered_map<int, uint32_t>;
	using ThreadHierarchyMap = std::unordered_map<std::thread::id, HierarchyMap>;
public:
	void Draw();

private:
	struct FrameData
	{
		ThreadHierarchyMap myThreadHierarchyMap;
		ThreadMarkMap myThreadMarkMap;
		std::unordered_map<std::thread::id, uint32_t> myMaxLevels;
		Profiler::FrameProfile myFrameProfile;
	};

	void DrawThreadColumn(const FrameData& aFrameData, float aTotalHeight) const;
	void DrawMarksColumn(const FrameData& aFrameData, float aTotalHeight) const;
	
	static HierarchyMap CalculateHierarchy(const MarksVec& aMarks, uint32_t& aMaxLevel);
	static FrameData ProcessFrameProfile(const Profiler::FrameProfile& aProfile);
	// converts a duration into larger units of time, them puts it
	// into a string buffer with a resulting unit of time
	// Warning: assumes aDuration is in nanosecods!
	static void DurationToString(char* aBuffer, long long aDuration);

	bool myShouldDraw = false;
	float myWidthScale = 1.f;
	std::vector<FrameData> myFramesToRender;
};