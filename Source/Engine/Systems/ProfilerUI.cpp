#include "Precomp.h"
#include "ProfilerUI.h"

#include "Input.h"
#include "Game.h"
#include "ImGUI/ImGUISystem.h"

#include <Core/Utils.h>

#include <sstream>

namespace 
{
	// converts a duration into larger units of time, them puts it
	// into a string buffer with a resulting unit of time
	// Warning: assumes aDuration is in nanosecods!
	template<uint32_t Length>
	void DurationToString(char(& aBuffer)[Length], long long aDuration)
	{
		uint32_t conversions = 0;
		long long remainder = 0;
		while (aDuration >= 1000 && conversions < 3)
		{
			remainder = aDuration % 1000;
			aDuration /= 1000;
			conversions++;
		}
		const char* unitOfTime;
		switch (conversions)
		{
		case 0: unitOfTime = "ns"; break;
		case 1: unitOfTime = "micros"; break;
		case 2: unitOfTime = "ms"; break;
		case 3: unitOfTime = "s"; break;
		default: ASSERT(false); break;
		}
		Utils::StringFormat(aBuffer, "%lld.%lld%s", aDuration, remainder, unitOfTime);
	}
}

ProfilerUI::ProfilerUI()
{
	Profiler::GetInstance().SetOnLongFrameCallback(
		[this](const Profiler::FrameProfile& aProfile) 
		{
			if (myAutoRecordLongFrames)
			{
				myFramesToRender.push_back(std::move(ProcessFrameProfile(aProfile)));
			}
		}
	);
}

void ProfilerUI::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("Profiler", &aIsOpen))
	{
		if (ImGui::Button("Buffer Init Frame"))
		{
			const auto& frameData = Profiler::GetInstance().GrabInitFrame();
			for (const Profiler::FrameProfile& frameProfile : frameData)
			{
				myFramesToRender.push_back(std::move(ProcessFrameProfile(frameProfile)));
			}
		}

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

		ImGui::Checkbox("Auto Record Long Frames?", &myAutoRecordLongFrames);

		bool plotWindowHovered = false;
		char nodeName[64];
		for (const FrameData& frameData : myFramesToRender)
		{
			Utils::StringFormat(nodeName, "Frame %llu", frameData.myFrameProfile.myFrameNum);
			if (ImGui::TreeNode(nodeName))
			{
				// Precalculate the whole height
				float totalHeight = kMarkHeight;
				for (const auto& threadMaxLevelPair : frameData.myMaxLevels)
				{
					totalHeight += (threadMaxLevelPair.second + 1) * kMarkHeight;
				}
				totalHeight += ImGui::GetStyle().ScrollbarSize;

				ImGui::BeginChild(nodeName, { 0,totalHeight });
				plotWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

				DrawThreadColumn(frameData, totalHeight);
				ImGui::SameLine();
				DrawMarksColumn(frameData, totalHeight);

				ImGui::EndChild();
				ImGui::TreePop();
			}
		}
		if (plotWindowHovered)
		{
			myWidthScale += Input::GetMouseWheelDelta() * 0.1f;
			myWidthScale = std::max(myWidthScale, 1.0f);
		}
	}

	ImGui::End();
}

void ProfilerUI::DrawThreadColumn(const FrameData& aFrameData, float aTotalHeight) const
{
	char name[64];
	Utils::StringFormat(name, "%llu##ThreadColumn", aFrameData.myFrameProfile.myFrameNum);
	ImGui::BeginChild(name, { kThreadColumnWidth, aTotalHeight });
	ImGui::SetCursorPosX(0);
	ImGui::SetNextItemWidth(kThreadColumnWidth);
	ImGui::Text("Thread Name");
	float yOffset = kMarkHeight;
	for (const auto& threadIdLevelPair : aFrameData.myMaxLevels)
	{
		const std::thread::id threadId = threadIdLevelPair.first;
		const uint32_t maxLevel = threadIdLevelPair.second;
		const float height = (maxLevel + 1) * kMarkHeight;

		ImGui::SetCursorPos({ 0, yOffset });
		std::ostringstream stringStream;
		stringStream << "Thread " << threadId;
		std::string threadString = stringStream.str();
		ImGui::Button(threadString.c_str(), { kThreadColumnWidth, height });
		yOffset += height;
	}
	ImGui::EndChild();
}

void ProfilerUI::DrawMarksColumn(const FrameData& aFrameData, float aTotalHeight) const
{
	// Unresizing scrollable parent window
	char name[64];
	char duration[32];
	Utils::StringFormat(name, "%llu##MarksColumn", aFrameData.myFrameProfile.myFrameNum);
	ImGui::BeginChild(name, { 0, aTotalHeight }, false, ImGuiWindowFlags_HorizontalScrollbar);

	// Zoomable child window
	const float fixedWindowWidth = ImGui::GetWindowWidth();
	float plotWidth = fixedWindowWidth * myWidthScale;
	plotWidth = std::max(plotWidth, fixedWindowWidth);
	Utils::StringFormat(name, "%llu##ZoomWindow", aFrameData.myFrameProfile.myFrameNum);
	ImGui::BeginChild(name, { plotWidth, aTotalHeight - ImGui::GetStyle().ScrollbarSize });

	// top bar for frame
	const long long frameDuration = (aFrameData.myFrameProfile.myEndStamp - aFrameData.myFrameProfile.myBeginStamp).count();
	DurationToString(duration, frameDuration);
	Utils::StringFormat(name, "Duration: %s", duration);
	ImGui::SetCursorPosX(0);
	ImGui::SetNextItemWidth(kThreadColumnWidth);
	ImGui::Button(name, { plotWidth, kMarkHeight });

	const long long frameStart = aFrameData.myFrameProfile.myBeginStamp.time_since_epoch().count();
	const long long frameEnd = aFrameData.myFrameProfile.myEndStamp.time_since_epoch().count();

	auto InverseLerpProfile = [min = frameStart, max = frameEnd](auto val) { 
		return (val - min) / static_cast<float>(max - min); 
	};

	constexpr ImU32 kColors[] = {
		IM_COL32(0, 128, 128, 255),
		IM_COL32(192, 128, 128, 255),
		IM_COL32(64, 128, 128, 255),
		IM_COL32(128, 0, 128, 255),
		IM_COL32(128, 192, 128, 255),
		IM_COL32(128, 64, 128, 255),
		IM_COL32(128, 128, 0, 255),
		IM_COL32(128, 128, 192, 255),
		IM_COL32(128, 128, 64, 255)
	};

	float yOffset = kMarkHeight;
	for (const auto& threadIdLevelPair : aFrameData.myMaxLevels)
	{
		const std::thread::id threadId = threadIdLevelPair.first;
		const uint32_t maxLevel = threadIdLevelPair.second;
		const float widthPerDurationRatio = plotWidth / frameDuration;
		
		const MarksVec& threadMarks = aFrameData.myThreadMarkMap.at(threadId);
		const HierarchyMap& hierarchy = aFrameData.myThreadHierarchyMap.at(threadId);
		for (const Profiler::Mark& mark : threadMarks)
		{
			const long long markDuration = (mark.myEndStamp - mark.myBeginStamp).count();
			const long long startTimeOffset = (mark.myBeginStamp - aFrameData.myFrameProfile.myBeginStamp).count();

			const float y = yOffset + hierarchy.at(mark.myId) * kMarkHeight;
			float x = plotWidth * InverseLerpProfile(mark.myBeginStamp.time_since_epoch().count());
			x = std::max(x, 0.f);
			float width = plotWidth - x; // by default, max possible
			if (markDuration)
			{
				// if we have a closed mark, then we can calculate actual width
				const float widthRatio = InverseLerpProfile(mark.myEndStamp.time_since_epoch().count());
				width = widthRatio * plotWidth - x;
			}
			ImGui::SetCursorPos({ x, y });

			const int colorInd = mark.myId % (sizeof(kColors) / sizeof(ImU32));
			ImGui::PushStyleColor(ImGuiCol_Button, kColors[colorInd]);

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
			ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_WHITE);

			if (markDuration)
			{
				DurationToString(duration, markDuration);
				Utils::StringFormat(name, "%s - %s", mark.myName, duration);
			}
			else
			{
				Utils::StringFormat(name, "%s - Ongoing", mark.myName);
			}
			ImGui::Button(name, { width, kMarkHeight });
			
			if (ImGui::IsItemHovered())
			{
				if (markDuration)
				{
					ImGui::SetTooltip("Name: %s\nDuration: %s\nId: %d\nParent Id: %d",
						mark.myName, duration, mark.myId, mark.myParentId);
				}
				else
				{
					ImGui::SetTooltip("Name: %s\nDuration: Ongoing\nId: %d\nParent Id: %d",
						mark.myName, mark.myId, mark.myParentId);
				}
			}
			ImGui::PopStyleVar();
			ImGui::PopStyleColor(2);
		}
		yOffset += (maxLevel + 1) * kMarkHeight;
	}
	ImGui::EndChild();
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
			uint32_t newLevel = hierarchyMap.at(mark.myParentId) + 1;
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
	for (auto& threadMarksPair : data.myThreadMarkMap)
	{
		ProfilerUI::MarksVec& threadMarks = threadMarksPair.second;
		// first, we need to sort the collection in order to be
		// able to construct a hierarchy map
		std::sort(threadMarks.begin(), threadMarks.end(), 
			[](const Profiler::Mark& aLeft, const Profiler::Mark& aRight) {
			return aLeft.myId < aRight.myId;
		});
		uint32_t maxLevel = 0;
		data.myThreadHierarchyMap[threadMarksPair.first] = std::move(CalculateHierarchy(threadMarks, maxLevel));
		data.myMaxLevels[threadMarksPair.first] = maxLevel;
	}
	data.myFrameProfile = std::move(aProfile);
	return data;
}

