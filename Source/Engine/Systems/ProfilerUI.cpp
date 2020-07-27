#include "Precomp.h"
#include "ProfilerUI.h"

#include "../Input.h"
#include <Core/Utils.h>

#include <sstream>

void ProfilerUI::Draw()
{
	if (Input::GetKeyPressed(Input::Keys::F2))
	{
		myShouldDraw = !myShouldDraw;
	}

	if (!myShouldDraw)
	{
		return;
	}

	ImGui::Begin("Profiler", &myShouldDraw);

	if (ImGui::Button("Buffer captures"))
	{
		const auto& frameData = Profiler::GetInstance().GetBufferedFrameData();
		for (const Profiler::FrameProfile& frameProfile : frameData)
		{
			myFramesToRender.push_back(std::move(ProcessFrameProfile(frameProfile)));
		}
	}
	if (ImGui::Button("Clear captures"))
	{
		myFramesToRender.clear();
	}

	bool plotWindowHovered = false;
	char nodeName[64];
	for (const FrameData& frameData : myFramesToRender)
	{
		sprintf(nodeName, "Frame %llu", frameData.myFrameProfile.myFrameNum);
		if (ImGui::TreeNode(nodeName))
		{
			/* TODO: so the current approach is just a prototype test, but it's already
			 * apparent that I can't continue the same way:
			 *	- I can't make left name column stick to border without scrolling
			*/

			// Precalculate the whole height
			float totalHeight = kMarkHeight + ImGui::GetStyle().ItemSpacing.y;
			for (const auto& threadMaxLevelPair : frameData.myMaxLevels)
			{
				totalHeight += (threadMaxLevelPair.second + 1) * kMarkHeight + ImGui::GetStyle().ItemSpacing.y;
			}
			totalHeight += ImGui::GetStyle().ScrollbarSize;

			ImGui::BeginChild(nodeName, { 0,totalHeight }, false, ImGuiWindowFlags_HorizontalScrollbar);
			plotWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
			DrawHeader(frameData.myFrameProfile);
			for (const auto& threadMarksPair : frameData.myThreadMarkMap)
			{
				DrawThreadRow(threadMarksPair.first, frameData);
			}
			ImGui::EndChild();
			ImGui::TreePop();
		}
	}
	if (plotWindowHovered)
	{
		myWidthScale += Input::GetMouseWheelDelta() * 0.1f;
		myWidthScale = std::max(myWidthScale, 1.0f);
	}

	ImGui::End();
}

void ProfilerUI::DrawHeader(const Profiler::FrameProfile& aProfile)
{
	const float windowWidth = ImGui::GetWindowWidth() * myWidthScale;
	const float plotWidth = windowWidth - kThreadColumnWidth;
	ImGui::SetCursorPosX(0);
	ImGui::PushItemWidth(kThreadColumnWidth);
	ImGui::Text("Thread");
	ImGui::PopItemWidth();
	ImGui::SameLine(kThreadColumnWidth);
	// time to draw custom rects!
	ImVec2 cursorPos = ImGui::GetCursorPos();
	int64_t frameDuration = (aProfile.myEndStamp - aProfile.myBeginStamp).count();
	char nameBuffer[64];
	std::sprintf(nameBuffer, "Duration: %lldns", frameDuration);
	ImGui::Button(nameBuffer, { plotWidth, kMarkHeight });
}

void ProfilerUI::DrawThreadRow(std::thread::id aThreadId, const FrameData& aFrameData)
{
	const float windowWidth = ImGui::GetWindowWidth() * myWidthScale;
	const float plotWidth = windowWidth - kThreadColumnWidth;
	std::ostringstream stringStream;
	stringStream << "Thread " << aThreadId;
	std::string threadString = stringStream.str();
	ImGui::SetCursorPosX(0);
	ImGui::Button(stringStream.str().c_str(), { kThreadColumnWidth, 0 });
	ImGui::SameLine(kThreadColumnWidth);

	const HierarchyMap& hierarchy = aFrameData.myThreadHierarchyMap.at(aThreadId);
	const uint32_t maxLevel = aFrameData.myMaxLevels.at(aThreadId);
	const float plotHeight = (maxLevel + 1) * kMarkHeight;
	
	// time to draw out all marks!
	ImGui::BeginChild(threadString.c_str(), { plotWidth, plotHeight });
	char name[64];
	const long long frameDuration = (aFrameData.myFrameProfile.myEndStamp - aFrameData.myFrameProfile.myBeginStamp).count();
	const float widthPerDurationRatio = plotWidth / frameDuration;
	auto InverseLerpProfile = [
		min = aFrameData.myFrameProfile.myBeginStamp.time_since_epoch().count(),
		max = aFrameData.myFrameProfile.myEndStamp.time_since_epoch().count()
	](auto val) {
		return (val - min) / static_cast<float>(max - min);
	};
	const MarksVec& threadMarks = aFrameData.myThreadMarkMap.at(aThreadId);
	for (const Profiler::Mark& mark : threadMarks)
	{
		const long long markDuration = (mark.myEndStamp - mark.myBeginStamp).count();
		const long long startTimeOffset = (mark.myBeginStamp - aFrameData.myFrameProfile.myBeginStamp).count();

		const float x = plotWidth * InverseLerpProfile(mark.myBeginStamp.time_since_epoch().count());
		const float y = hierarchy.at(mark.myId) * kMarkHeight;
		const float width = widthPerDurationRatio * markDuration;
		ImGui::SetCursorPos({ x, y });
		
		std::sprintf(name, "%s - %lldns", mark.myName, markDuration);
		ImGui::Button(name, { width, kMarkHeight });
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Name: %s\nDuration: %lldns\nId: %d\nParent Id: %d",
				mark.myName, markDuration, mark.myId, mark.myParentId);
		}
	}
	ImGui::EndChild();
}

ProfilerUI::HierarchyMap ProfilerUI::CalculateHierarchy(const MarksVec& aMarks, uint32_t& aMaxLevel)
{
	// we need to figure out how to display the stacked bars representing
	// marks, so for that we will calculate the nested-level of every Mark
	HierarchyMap hierarchyMap;
	hierarchyMap.reserve(aMarks.size());
	// While we're at it, calculate the max nested level, so that we can determine
	// the height of hierarchy
	aMaxLevel = 0;
	for (const Profiler::Mark& mark : aMarks)
	{
		if (mark.myParentId == -1)
		{
			// no parent -> initial level
			hierarchyMap[mark.myId] = 0;
		}
		else
		{
			// parent exists -> parents level + 1
			uint32_t newLevel = hierarchyMap[mark.myParentId] + 1;
			hierarchyMap[mark.myId] = newLevel;
			// update the max level
			aMaxLevel = std::max(aMaxLevel, newLevel);
		}
	}
	return hierarchyMap;
}

ProfilerUI::FrameData ProfilerUI::ProcessFrameProfile(const Profiler::FrameProfile& aProfile)
{
	FrameData data;
	data.myThreadMarkMap = Utils::GroupBy<ThreadMarkMap>(aProfile.myFrameMarks,
		[](const Profiler::Mark& aMark) { return aMark.myThreadId; }
	);
	for (const auto& threadMarksPair : data.myThreadMarkMap)
	{
		uint32_t maxLevel = 0;
		data.myThreadHierarchyMap[threadMarksPair.first] = std::move(CalculateHierarchy(threadMarksPair.second, maxLevel));
		data.myMaxLevels[threadMarksPair.first] = maxLevel;
	}
	data.myFrameProfile = std::move(aProfile);
	return data;
}