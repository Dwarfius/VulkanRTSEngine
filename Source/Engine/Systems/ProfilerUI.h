#pragma once

#include <Core/Profiler.h>

class ProfilerUI
{
	constexpr static float kThreadColumnWidth = 80.f;
	using MarksVec = std::vector<Profiler::Mark>;
	using ThreadMarkMap = std::unordered_map<std::thread::id, MarksVec>;
	using HierarchyMap = std::unordered_map<int, uint32_t>;
public:
	void Draw();

private:
	ThreadMarkMap GroupBy(const std::vector<Profiler::Mark>& aMarks) const;
	void DrawHeader(const Profiler::FrameProfile& aProfile);
	void DrawThreadRow(std::thread::id aThreadId, const MarksVec& aThreadMarks, const Profiler::FrameProfile& aProfile);
	HierarchyMap CalculateHierarchy(const MarksVec& aMarks, uint32_t& aMaxLevel) const;

	bool myShouldDraw = false;
	float myWidthScale = 1.f;
	std::vector<Profiler::FrameProfile> myFramesToRender;
};