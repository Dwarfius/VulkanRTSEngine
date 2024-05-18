#include "Precomp.h"
#include "ProfilerUI.h"

#include "Input.h"
#include "Game.h"
#include "ImGUI/ImGUISystem.h"

#include <Core/Utils.h>
#include <Core/Profiler.h>

#include <sstream>
#include <imgui_internal.h>

// Taken and adapted from https://github.com/ocornut/imgui/issues/3379#issuecomment-669259429
void ScrollWhenDraggingOnVoid(const glm::vec2& aDelta, uint32_t aButton)
{
	const ImGuiContext& g = *ImGui::GetCurrentContext();
	ImGuiWindow* window = g.CurrentWindow;
	const ImGuiID overlayId = window->GetID("##scrolldraggingoverlay");
	bool hovered = false;
	if (g.HoveredId == 0 || g.HoveredId == overlayId)
	{
		const ImGuiButtonFlags buttonFlags = (aButton == 0)
			? ImGuiButtonFlags_MouseButtonLeft : (aButton == 1)
			? ImGuiButtonFlags_MouseButtonRight : ImGuiButtonFlags_MouseButtonMiddle;
		ImGui::ButtonBehavior(window->Rect(), overlayId, &hovered, nullptr, buttonFlags);
	}
	
	if (hovered // are we still on the invisible button?
		// or any child elem of the window containg it?
		|| (g.HoveredWindow && g.HoveredWindow->ID == g.CurrentWindow->ID)) 
	{
		const bool held = ImGui::GetIO().MouseDown[aButton];
		if (held && aDelta.x != 0.0f)
		{
			ImGui::SetScrollX(window, window->Scroll.x - aDelta.x);
		}
		if (held && aDelta.y != 0.0f)
		{
			ImGui::SetScrollY(window, window->Scroll.y - aDelta.y);
		}
	}
}

namespace 
{
	float InverseLerpProfile(uint64_t val, uint64_t min, uint64_t max) 
	{
		return (val - min) / static_cast<float>(max - min);
	};

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
		Utils::StringFormat(aBuffer, "{}.{}{}", aDuration, remainder, unitOfTime);
	}

	ProfilerUI::FrameData ProcessFrameProfile(const Profiler::FrameProfile& aProfile)
	{
		std::unordered_map<uint32_t, ProfilerUI::Mark> marksMap;

		for (const Profiler::ThreadProfile& threadProfile : aProfile.myThreadProfiles)
		{
			for (const Profiler::Mark& startMark : threadProfile.myStartMarks)
			{
				ProfilerUI::Mark mark;
				mark.myId = startMark.myId;
				mark.myThreadId = threadProfile.myThreadId;
				mark.myStart = startMark.myStamp;
				mark.myEnd = aProfile.myEndStamp;
				mark.myDepth = startMark.myDepth;
				std::memcpy(mark.myName, startMark.myName, sizeof(startMark.myName));
				marksMap.insert({ startMark.myId, mark });
			}

			for (const Profiler::Mark& endMark : threadProfile.myEndMarks)
			{
				// we either have mark to "close"
				// or it's a new one from previous frames
				auto iter = marksMap.find(endMark.myId);
				if (iter != marksMap.end())
				{
					iter->second.myEnd = endMark.myStamp;
				}
				else
				{
					ProfilerUI::Mark mark;
					mark.myId = endMark.myId;
					mark.myThreadId = threadProfile.myThreadId;
					mark.myStart = aProfile.myBeginStamp;
					mark.myEnd = endMark.myStamp;
					mark.myDepth = endMark.myDepth;
					std::memcpy(mark.myName, endMark.myName, sizeof(endMark.myName));
					marksMap.insert({ endMark.myId, mark });
				}
			}
		}

		ProfilerUI::FrameData data;
		for (const auto& [id, mark] : marksMap)
		{
			data.myThreadMarkMap[mark.myThreadId].push_back(mark);
		}

		for (const auto& [threadId, markVec] : data.myThreadMarkMap)
		{
			uint8_t maxLevel = 0;
			for (const ProfilerUI::Mark& mark : markVec)
			{
				maxLevel = std::max(maxLevel, mark.myDepth);
			}
			data.myMaxLevels[threadId] = maxLevel;
		}
		data.myFrameStart = aProfile.myBeginStamp;
		data.myFrameEnd = aProfile.myEndStamp;
		data.myFrameNum = aProfile.myFrameNum;
		return data;
	}
}

ProfilerUI::ProfilerUI()
{
	Profiler::GetInstance().SetIsFrameReportingEnabled(true);
	Profiler::GetInstance().SetOnLongFrameCallback(
		[this](const Profiler::FrameProfile& aProfile) 
		{
			myFrames.push_back(std::move(ProcessFrameProfile(aProfile)));
			myNeedsToUpdateScopeData = true;
			UpdateThreadMapping();
		}
	);
}

void ProfilerUI::Draw(bool& aIsOpen)
{
	if (ImGui::Begin("Profiler", &aIsOpen))
	{
		if (ImGui::Button("Buffer Init Frame"))
		{
			myInitFrames.clear();
			Profiler::GetInstance().GatherInitFrames(
				[this](const Profiler::FrameProfile& aFrame)
				{
					myInitFrames.push_back(std::move(ProcessFrameProfile(aFrame)));
				}
			);
			myNeedsToUpdateScopeData = true;
			UpdateThreadMapping();

			// force zoom to be recalculated
			myInitFramesScale = std::numeric_limits<float>::min();
		}

		ImGui::SameLine();
		if (ImGui::Button("Buffer captures"))
		{
			Profiler::GetInstance().GatherBufferedFrames(
				[this](const Profiler::FrameProfile& aFrame)
				{
					myFrames.push_back(std::move(ProcessFrameProfile(aFrame)));
				}
			);
			myNeedsToUpdateScopeData = true;
			UpdateThreadMapping();

			// force zoom to be recalculated
			myFramesScale = std::numeric_limits<float>::min();
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear captures"))
		{
			myFrames.clear();
			myNeedsToUpdateScopeData = true;
		}

		if(ImGui::Checkbox("Auto Record Long Frames?", &myAutoRecordLongFrames))
		{
			Profiler::GetInstance().SetIsFrameReportingEnabled(myAutoRecordLongFrames);
		}

		if (ImGui::TreeNode("Scope Tracking"))
		{
			DrawScopesView();
			ImGui::TreePop();
		}
		if (!myInitFrames.empty() 
			&& ImGui::TreeNode("Frame Tracking - Initial Frames"))
		{
			DrawFrames(myInitFrames, myInitFramesScale);
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Frame Tracking - Recent Frames"))
		{
			DrawFrames(myFrames, myFramesScale);
			ImGui::TreePop();
		}
	}

	ImGui::End();
}

void ProfilerUI::DrawScopesView()
{
	ImGui::Text("Tracked Scopes");
	char buffer[64];
	for(size_t i=0; i<myScopeNames.size(); i++)
	{
		std::string& scope = myScopeNames[i];
		Utils::StringFormat(buffer, "##{}", i);
		myNeedsToUpdateScopeData |= ImGui::InputText(buffer, scope);

		ImGui::SameLine();
		Utils::StringFormat(buffer, "Delete##{}", i);
		if (ImGui::Button(buffer))
		{
			myScopeNames.erase(myScopeNames.begin() + i);
			i--;
			myNeedsToUpdateScopeData = true;
		}
	}
	if (ImGui::Button("Add Scope"))
	{
		myScopeNames.emplace_back();
		myNeedsToUpdateScopeData = true;
	}

	if (myNeedsToUpdateScopeData)
	{
		myScopeData.clear();
		myScopeData.resize(myScopeNames.size());
		auto ProcessFrameData = [](const FrameData& aFrameData, ScopeData& aScopeData) {
			bool foundInFrame = false;
			for (const auto& [thread, marksVec] : aFrameData.myThreadMarkMap)
			{
				for (const Mark& mark : marksVec)
				{
					const std::string_view nameView = mark.myName;
					if (nameView == aScopeData.myScopeName)
					{
						foundInFrame = true;
						aScopeData.myTotalCount++;
						const uint64_t durationNs = mark.myEnd - mark.myStart;
						aScopeData.myMin = glm::min(durationNs, aScopeData.myMin);
						aScopeData.myMax = glm::max(durationNs, aScopeData.myMax);
						aScopeData.myTotalAvg += durationNs;
					}
				}
			}

			if (foundInFrame)
			{
				aScopeData.myFoundInFrameCount++;
			}
		};
		
		for (size_t i = 0; i < myScopeNames.size(); i++)
		{
			ScopeData& scope = myScopeData[i];
			scope.myScopeName = myScopeNames[i];

			for (const FrameData& frameData : myInitFrames)
			{
				ProcessFrameData(frameData, scope);
			}

			for (const FrameData& frameData : myFrames)
			{
				ProcessFrameData(frameData, scope);
			}

			if (scope.myTotalCount)
			{
				scope.myAvgPerFrameCount = scope.myTotalCount / scope.myFoundInFrameCount;
				scope.myMedian = scope.myMin + (scope.myMax - scope.myMin) / 2;
				scope.myTotalAvg /= scope.myFoundInFrameCount;
			}
		}
		myNeedsToUpdateScopeData = false;
	}

	ImGui::Separator();
	if (ImGui::BeginTable("Scopes", 7, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("# Total");
		ImGui::TableSetupColumn("# Avg Per Frame");
		ImGui::TableSetupColumn("Avg Frame Total");
		ImGui::TableSetupColumn("Min");
		ImGui::TableSetupColumn("Max");
		ImGui::TableSetupColumn("Median");
		ImGui::TableHeadersRow();
		ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
		if (sortSpecs && sortSpecs->SpecsDirty)
		{
			int count = sortSpecs->SpecsCount;
			for (int i = 0; i < sortSpecs->SpecsCount; i++)
			{
				const ImGuiTableColumnSortSpecs& sortSpec = sortSpecs->Specs[i];

				// We only support 1 level of sorting
				if (sortSpec.SortOrder == 0)
				{
					bool asc = sortSpec.SortDirection == ImGuiSortDirection_Ascending;
					switch (sortSpec.ColumnIndex)
					{
					case 0: // Name
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc 
								? aLeft.myScopeName < aRight.myScopeName 
								: aLeft.myScopeName > aRight.myScopeName;
						}
						);
						break;
					case 1: // # Total
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc
								? aLeft.myTotalCount < aRight.myTotalCount
								: aLeft.myTotalCount > aRight.myTotalCount;
						}
						);
						break;
					case 2: // # Avg Per Frame
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc
								? aLeft.myAvgPerFrameCount < aRight.myAvgPerFrameCount
								: aLeft.myAvgPerFrameCount > aRight.myAvgPerFrameCount;
						}
						);
						break;
					case 3: // Avg Frame Total
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc
								? aLeft.myTotalAvg < aRight.myTotalAvg
								: aLeft.myTotalAvg > aRight.myTotalAvg;
						}
						);
						break;
					case 4: // Min
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc 
								? aLeft.myMin < aRight.myMin
								: aLeft.myMin > aRight.myMin;
						}
						);
						break;
					case 5: // Max
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc
								? aLeft.myMax < aRight.myMax
								: aLeft.myMax > aRight.myMax;
						}
						);
						break;
					case 6: // Median
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc
								? aLeft.myMedian < aRight.myMedian
								: aLeft.myMedian > aRight.myMedian;
						}
						);
						break;
					default:
						ASSERT(false);
					}
				}
			}
			sortSpecs->SpecsDirty = false;
		}

		for (const ScopeData& scope : myScopeData)
		{
			ImGui::TableNextRow();
			if (ImGui::TableNextColumn())
			{
				ImGui::Text(scope.myScopeName.data());
			}
			if (ImGui::TableNextColumn())
			{
				Utils::StringFormat(buffer, "{}", scope.myTotalCount);
				ImGui::Text(buffer);
			}
			if (ImGui::TableNextColumn())
			{
				Utils::StringFormat(buffer, "{}", scope.myAvgPerFrameCount);
				ImGui::Text(buffer);
			}
			if (ImGui::TableNextColumn())
			{
				DurationToString(buffer, scope.myTotalAvg);
				ImGui::Text(buffer);
			}
			if (ImGui::TableNextColumn())
			{
				DurationToString(buffer, scope.myMin);
				ImGui::Text(buffer);
			}
			if (ImGui::TableNextColumn())
			{
				DurationToString(buffer, scope.myMax);
				ImGui::Text(buffer);
			}
			if (ImGui::TableNextColumn())
			{
				DurationToString(buffer, scope.myMedian);
				ImGui::Text(buffer);
			}
		}
	}
	ImGui::EndTable();
}

void ProfilerUI::DrawThreadColumn(float aMarkHeight, float aTotalHeight) const
{
	ImGui::SetCursorPosX(0);
	ImGui::SetNextItemWidth(kThreadColumnWidth);
	ImGui::Text("Thread Name");
	for (const auto& [id, maxLevel, levelOffset] : myThreadMapping)
	{
		const float height = (maxLevel + 1) * aMarkHeight;

		// +1 saved to the title row of "Frame:Duration"
		ImGui::SetCursorPos({ 0, aMarkHeight * (levelOffset + 1) });
		
		std::ostringstream stringStream;
		stringStream << "T" << id;
		std::string threadString = stringStream.str();
		ImGui::Button(threadString.c_str(), { kThreadColumnWidth, height });
	}
}

void ProfilerUI::DrawFrames(std::span<FrameData> aData, float& aScale) const
{
	const float markHeight = ImGui::GetFrameHeight();

	// Precalculate the whole height
	float totalHeight = markHeight; // title row bar
	if (!myThreadMapping.empty())
	{
		const ThreadInfo& lastInfo = myThreadMapping.back();
		totalHeight += (lastInfo.myAboveMaxLevel + lastInfo.myMaxLevel + 1) * markHeight;
	}
	totalHeight += ImGui::GetStyle().ScrollbarSize;

	ImGui::BeginChild("Scrollable", { 0,totalHeight }, false, ImGuiWindowFlags_HorizontalScrollbar);
	const bool plotWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	DrawThreadColumn(markHeight, totalHeight);

	float xOffset = kThreadColumnWidth;
	for (const FrameData& frameData : aData)
	{
		const size_t durationNs = (frameData.myFrameEnd - frameData.myFrameStart);
		const float frameWidth = kPixelsPerMs * (durationNs / 1'000'000.f) * aScale;
		const glm::vec2 startPos = { xOffset, 0 };
		DrawFrameMarks(frameData, startPos, markHeight, frameWidth);
		xOffset += frameWidth;
	}
	const ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
	ScrollWhenDraggingOnVoid({ mouseDelta.x, 0 }, ImGuiMouseButton_Left);
	const float clipRectW = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
	ImGui::EndChild();

	const float mouseWheelDelta = Input::GetMouseWheelDelta();
	if (aScale == std::numeric_limits<float>::min() 
		|| (plotWindowHovered && mouseWheelDelta))
	{
		aScale += mouseWheelDelta * 0.1f;

		// Prevent shrinking of the view
		const float contentWidth = clipRectW - kThreadColumnWidth;
		const float maxZoomOut = CalcMaxZoomOut(aData, contentWidth);
		aScale = std::max(aScale, maxZoomOut);
	}
}

void ProfilerUI::DrawFrameMarks(const FrameData& aFrameData, glm::vec2 aPos, float aMarkHeight, float aFrameWidth) const
{
	char name[64];
	
	// top bar for frame
	char duration[32];
	const uint64_t frameDuration = aFrameData.myFrameEnd - aFrameData.myFrameStart;
	DurationToString(duration, frameDuration);
	Utils::StringFormat(name, "{}: {}", aFrameData.myFrameNum, duration);
	ImGui::SetCursorPosX(aPos.x);
	ImGui::SetCursorPosY(aPos.y);
	ImGui::SetNextItemWidth(kThreadColumnWidth);
	ImGui::Button(name, { aFrameWidth, aMarkHeight });

	const uint64_t frameStart = aFrameData.myFrameStart;
	const uint64_t frameEnd = aFrameData.myFrameEnd;

	// TODO: try to make colors stick to same marks, to make it easy
	// to see it across frames
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

	for (const auto& [threadId, threadMarks] : aFrameData.myThreadMarkMap)
	{
		const auto mappingIter = std::find_if(myThreadMapping.begin(), myThreadMapping.end(),
			[threadId](const auto& anItem) {
				return anItem.myId == threadId;
			}
		);
		ASSERT_STR(mappingIter != myThreadMapping.end(), "Out of date thread mapping!");
		const ThreadInfo& info = *mappingIter;
		const float yOffset = info.myAboveMaxLevel * aMarkHeight + aMarkHeight;
		for (const Mark& mark : threadMarks)
		{
			const int colorInd = mark.myId % (sizeof(kColors) / sizeof(ImU32));
			
			DrawMark(mark, aPos + glm::vec2{0, yOffset }, aFrameWidth, aMarkHeight, kColors[colorInd], {frameStart, frameEnd});
		}
	}
}

void ProfilerUI::DrawMark(const Mark& aMark, glm::vec2 aPos, float aFrameWidth, float aMarkHeight, ImU32 aColor, glm::u64vec2 aFrameTimes) const
{
	const uint64_t frameStart = aFrameTimes.x;
	const uint64_t frameEnd = aFrameTimes.y;

	const uint64_t markDuration = aMark.myEnd - aMark.myStart;
	const uint64_t startTimeOffset = aMark.myStart - frameStart;
	
	const float y = aMark.myDepth * aMarkHeight;
	const float x = aFrameWidth * InverseLerpProfile(aMark.myStart, frameStart, frameEnd);
	float width = aFrameWidth - x; // by default, max possible

	const bool isMarkFinished = aMark.myEnd != frameEnd;
	if (isMarkFinished)
	{
		// if we have a closed mark, then we can calculate actual width
		const float widthRatio = InverseLerpProfile(aMark.myEnd, frameStart, frameEnd);
		width = widthRatio * aFrameWidth - x;
	}
	// Don't render out marks that are too small to see at current zoom level
	// This'll mean that users might not spot it. But, since we have Scopes view,
	// they'll be able to filter for it
	if (width < 1.f)
	{
		return;
	}

	ImGui::SetCursorPos({ aPos.x + x, aPos.y + y });

	ImGui::PushStyleColor(ImGuiCol_Button, aColor);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_WHITE);

	char name[64];
	char duration[32];
	if (isMarkFinished)
	{
		DurationToString(duration, markDuration);
		Utils::StringFormat(name, "{} - {}", aMark.myName, duration);
	}
	else
	{
		Utils::StringFormat(name, "{} - Ongoing", aMark.myName);
	}
	ImGui::Button(name, { width, aMarkHeight });

	if (ImGui::IsItemHovered())
	{
		if (isMarkFinished)
		{
			ImGui::SetTooltip("Name: %s\nDuration: %s",
				aMark.myName, duration);
		}
		else
		{
			ImGui::SetTooltip("Name: %s\nDuration: Ongoing",
				aMark.myName);
		}
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor(2);
}

void ProfilerUI::UpdateThreadMapping()
{
	myThreadMapping.clear(); // rebuild the whole mapping, as it's pretty cheap

	auto ProcessFrameData = [&](const FrameData& aFrameData) {
		for (const auto& [threadId, maxLevel] : aFrameData.myMaxLevels)
		{
			const auto iter = std::find_if(myThreadMapping.begin(), myThreadMapping.end(),
				[threadId](const auto& anItem)
			{
				return anItem.myId == threadId;
			}
			);
			if (iter != myThreadMapping.end())
			{
				iter->myMaxLevel = glm::max(maxLevel, iter->myMaxLevel);
			}
			else
			{
				myThreadMapping.push_back({ threadId, maxLevel });
			}
		}
	};

	for (const FrameData& frameData : myInitFrames)
	{
		ProcessFrameData(frameData);
	}
	for (const FrameData& frameData : myFrames)
	{
		ProcessFrameData(frameData);
	}

	// Precalculate how many "levels" of marks there will be for specific thread
	// down the list.
	for (size_t i = 1; i < myThreadMapping.size(); i++)
	{
		myThreadMapping[i].myAboveMaxLevel = 
			myThreadMapping[i - 1].myMaxLevel + 1 + myThreadMapping[i - 1].myAboveMaxLevel;
	}
}

float ProfilerUI::CalcMaxZoomOut(std::span<FrameData> aData, float aWidth)
{
	float durationMs = 0;
	for (const FrameData& frameData : aData)
	{
		const size_t durationNs = (frameData.myFrameEnd - frameData.myFrameStart);
		durationMs += durationNs / 1'000'000.f;
	}
	return aWidth / (durationMs * kPixelsPerMs);
}