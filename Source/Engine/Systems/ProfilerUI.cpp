#include "Precomp.h"
#include "ProfilerUI.h"

#include "../Input.h"
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
		myFramesToRender.insert(myFramesToRender.end(), frameData.begin(), frameData.end());
	}
	if (ImGui::Button("Clear captures"))
	{
		myFramesToRender.clear();
	}

	bool plotWindowHovered = false;
	char nodeName[64];
	for (const Profiler::FrameProfile& frameProfile : myFramesToRender)
	{
		sprintf(nodeName, "Frame %llu", frameProfile.myFrameNum);
		if (ImGui::TreeNode(nodeName))
		{
			/* TODO: so the current approach is just a prototype test, but it's already
			 * apparent that I can't continue the same way:
			 *	- I can't make left name column stick to border without scrolling
			 *  - I can't predict the vertical size of graph, meaning I can't 
			 *    prevent child window autofil
			*/
			ImGui::BeginChild(nodeName, { 0,0 }, false, ImGuiWindowFlags_HorizontalScrollbar);
			plotWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
			DrawHeader(frameProfile);
			ThreadMarkMap threadMarksMap = GroupBy(frameProfile.myFrameMarks);
			for (const auto& threadMarksPair : threadMarksMap)
			{
				DrawThreadRow(threadMarksPair.first, threadMarksPair.second, frameProfile);
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

// TODO: generalise and move this to Utils
ProfilerUI::ThreadMarkMap ProfilerUI::GroupBy(const std::vector<Profiler::Mark>& aMarks) const
{
	ThreadMarkMap map;
	for (const Profiler::Mark& mark : aMarks)
	{
		auto iter = map.find(mark.myThreadId);
		if (iter == map.end())
		{
			std::vector newBuffer{ mark };
			map[mark.myThreadId] = std::move(newBuffer);
		}
		else
		{
			iter->second.push_back(mark);
		}
	}
	return map;
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
	ImGui::Button(nameBuffer, { plotWidth, 0 });
}

void ProfilerUI::DrawThreadRow(std::thread::id aThreadId, const MarksVec& aThreadMarks, const Profiler::FrameProfile& aProfile)
{
	const float windowWidth = ImGui::GetWindowWidth() * myWidthScale;
	const float plotWidth = windowWidth - kThreadColumnWidth;
	std::ostringstream stringStream;
	stringStream << "Thread " << aThreadId;
	std::string threadString = stringStream.str();
	ImGui::SetCursorPosX(0);
	ImGui::Button(stringStream.str().c_str(), { kThreadColumnWidth, 0 });
	ImGui::SameLine(kThreadColumnWidth);

	uint32_t maxLevel = 0;
	// TODO: This can be done on ProfileData buffering! This will also help
	// to determine a lot of dimension ahead of the curve (which is needed)
	// to avoid auto-filling windows
	HierarchyMap hierarchy = CalculateHierarchy(aThreadMarks, maxLevel);
	constexpr float kMarkHeight = 30.f;
	const float plotHeight = (maxLevel + 1) * kMarkHeight;
	// time to draw out all marks!
	ImGui::BeginChild(threadString.c_str(), { plotWidth, plotHeight });
	char name[64];
	const long long frameDuration = (aProfile.myEndStamp - aProfile.myBeginStamp).count();
	const float widthPerDurationRatio = plotWidth / frameDuration;
	auto InverseLerpProfile = [
		min = aProfile.myBeginStamp.time_since_epoch().count(), 
		max = aProfile.myEndStamp.time_since_epoch().count()
	](auto val) {
		return (val - min) / static_cast<float>(max - min);
	};
	for (const Profiler::Mark& mark : aThreadMarks)
	{
		const long long markDuration = (mark.myEndStamp - mark.myBeginStamp).count();
		const long long startTimeOffset = (mark.myBeginStamp - aProfile.myBeginStamp).count();

		const float x = plotWidth * InverseLerpProfile(mark.myBeginStamp.time_since_epoch().count());
		const float y = hierarchy[mark.myId] * kMarkHeight;
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

ProfilerUI::HierarchyMap ProfilerUI::CalculateHierarchy(const MarksVec& aMarks, uint32_t& aMaxLevel) const
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
