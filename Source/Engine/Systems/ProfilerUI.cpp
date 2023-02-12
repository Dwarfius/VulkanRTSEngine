#include "Precomp.h"
#include "ProfilerUI.h"

#include "Input.h"
#include "Game.h"
#include "ImGUI/ImGUISystem.h"

#include <Core/Utils.h>
#include <Core/Profiler.h>

#include <sstream>

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
		Utils::StringFormat(aBuffer, "%lld.%lld%s", aDuration, remainder, unitOfTime);
	}

	ProfilerUI::FrameData ProcessFrameProfile(const Profiler::FrameProfile& aProfile)
	{
		std::unordered_map<uint32_t, ProfilerUI::Mark> marksMap;

		for (const Profiler::Mark& startMark : aProfile.myStartMarks)
		{
			ProfilerUI::Mark mark;
			mark.myId = startMark.myId;
			mark.myThreadId = startMark.myThreadId;
			mark.myStart = startMark.myStamp;
			mark.myEnd = aProfile.myEndStamp;
			mark.myDepth = startMark.myDepth;
			std::memcpy(mark.myName, startMark.myName, sizeof(startMark.myName));
			marksMap.insert({ startMark.myId, mark });
		}

		for (const Profiler::Mark& endMark : aProfile.myEndMarks)
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
				mark.myThreadId = endMark.myThreadId;
				mark.myStart = aProfile.myBeginStamp;
				mark.myEnd = endMark.myStamp;
				mark.myDepth = endMark.myDepth;
				std::memcpy(mark.myName, endMark.myName, sizeof(endMark.myName));
				marksMap.insert({ endMark.myId, mark });
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
	Profiler::GetInstance().SetOnLongFrameCallback(
		[this](const Profiler::FrameProfile& aProfile) 
		{
			if (myAutoRecordLongFrames)
			{
				myFramesToRender.push_back(std::move(ProcessFrameProfile(aProfile)));
				myNeedsToUpdateScopeData = true;
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
				myNeedsToUpdateScopeData = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Buffer captures"))
		{
			const auto& frameData = Profiler::GetInstance().GetBufferedFrameData();
			for (const Profiler::FrameProfile& frameProfile : frameData)
			{
				myFramesToRender.push_back(std::move(ProcessFrameProfile(frameProfile)));
				myNeedsToUpdateScopeData = true;
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Clear captures"))
		{
			myFramesToRender.clear();
			myNeedsToUpdateScopeData = true;
		}

		ImGui::Checkbox("Auto Record Long Frames?", &myAutoRecordLongFrames);

		if (ImGui::TreeNode("Scope Tracking"))
		{
			DrawScopesView();
			ImGui::TreePop();
		}
		if (ImGui::TreeNode("Frame Tracking"))
		{
			bool plotWindowHovered = false;
			char nodeName[64];
			
			const float markHeight = ImGui::GetFrameHeight();
			for (const FrameData& frameData : myFramesToRender)
			{
				Utils::StringFormat(nodeName, "Frame %llu", frameData.myFrameNum);
				if (ImGui::TreeNode(nodeName))
				{
					// Precalculate the whole height
					float totalHeight = markHeight;
					for (const auto& threadMaxLevelPair : frameData.myMaxLevels)
					{
						totalHeight += (threadMaxLevelPair.second + 1) * markHeight;
					}
					totalHeight += ImGui::GetStyle().ScrollbarSize;

					ImGui::BeginChild(nodeName, { 0,totalHeight });
					plotWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

					DrawThreadColumn(frameData, markHeight, totalHeight);
					ImGui::SameLine();
					DrawMarksColumn(frameData, markHeight, totalHeight);

					ImGui::EndChild();
					ImGui::TreePop();
				}
			}
			if (plotWindowHovered)
			{
				myWidthScale += Input::GetMouseWheelDelta() * 0.1f;
				myWidthScale = std::max(myWidthScale, 1.0f);
			}
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
		Utils::StringFormat(buffer, "##%llu", i);
		myNeedsToUpdateScopeData |= ImGui::InputText(buffer, scope.data(),
			scope.capacity() + 1, ImGuiInputTextFlags_CallbackResize,
			[](ImGuiInputTextCallbackData* aData)
			{
				std::string* valueStr = static_cast<std::string*>(aData->UserData);
				if (aData->EventFlag == ImGuiInputTextFlags_CallbackResize)
				{
					valueStr->resize(aData->BufTextLen);
					aData->Buf = valueStr->data();
				}
				return 0;
			}, &scope
		);

		ImGui::SameLine();
		Utils::StringFormat(buffer, "Delete##%llu", i);
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
		for (size_t i = 0; i < myScopeNames.size(); i++)
		{
			ScopeData& scope = myScopeData[i];
			const std::string& scopeName = myScopeNames[i];
			scope.myScopeName = scopeName;

			for (const FrameData& frameData : myFramesToRender)
			{
				bool foundInFrame = false;
				for (const auto& [thread, marksVec] : frameData.myThreadMarkMap)
				{
					for (const Mark& mark : marksVec)
					{
						const std::string_view nameView = mark.myName;
						if (nameView == scopeName)
						{
							foundInFrame = true;
							scope.myTotalCount++;
							const uint64_t durationNs = mark.myEnd - mark.myStart;
							scope.myMin = glm::min(durationNs, scope.myMin);
							scope.myMax = glm::max(durationNs, scope.myMax);
						}
					}
				}

				if (foundInFrame)
				{
					scope.myFoundInFrameCount++;
				}
			}

			if (scope.myTotalCount)
			{
				scope.myAvgPerFrameCount = scope.myTotalCount / scope.myFoundInFrameCount;
				scope.myMedian = scope.myMin + (scope.myMax - scope.myMin) / 2;
			}
		}
		myNeedsToUpdateScopeData = false;
	}

	ImGui::Separator();
	if (ImGui::BeginTable("Scopes", 6, ImGuiTableFlags_Sortable | ImGuiTableFlags_SizingStretchProp))
	{
		ImGui::TableSetupColumn("Name");
		ImGui::TableSetupColumn("Total");
		ImGui::TableSetupColumn("Avg Per Frame");
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
					case 1: // Total
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc
								? aLeft.myTotalCount < aRight.myTotalCount
								: aLeft.myTotalCount > aRight.myTotalCount;
						}
						);
						break;
					case 2: // Avg Per Frame
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc
								? aLeft.myAvgPerFrameCount < aRight.myAvgPerFrameCount
								: aLeft.myAvgPerFrameCount > aRight.myAvgPerFrameCount;
						}
						);
						break;
					case 3: // Min
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc 
								? aLeft.myMin < aRight.myMin
								: aLeft.myMin > aRight.myMin;
						}
						);
						break;
					case 4: // Max
						std::sort(myScopeData.begin(), myScopeData.end(),
							[asc](const auto& aLeft, const auto& aRight)
						{
							return asc
								? aLeft.myMax < aRight.myMax
								: aLeft.myMax > aRight.myMax;
						}
						);
						break;
					case 5: // Median
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
				Utils::StringFormat(buffer, "%llu", scope.myTotalCount);
				ImGui::Text(buffer);
			}
			if (ImGui::TableNextColumn())
			{
				Utils::StringFormat(buffer, "%llu", scope.myAvgPerFrameCount);
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

void ProfilerUI::DrawThreadColumn(const FrameData& aFrameData, float aMarkHeight, float aTotalHeight) const
{
	char name[64];
	Utils::StringFormat(name, "%llu##ThreadColumn", aFrameData.myFrameNum);
	ImGui::BeginChild(name, { kThreadColumnWidth, aTotalHeight });
	ImGui::SetCursorPosX(0);
	ImGui::SetNextItemWidth(kThreadColumnWidth);
	ImGui::Text("Thread Name");
	float yOffset = aMarkHeight;
	for (const auto& threadIdLevelPair : aFrameData.myMaxLevels)
	{
		const std::thread::id threadId = threadIdLevelPair.first;
		const uint32_t maxLevel = threadIdLevelPair.second;
		const float height = (maxLevel + 1) * aMarkHeight;

		ImGui::SetCursorPos({ 0, yOffset });
		std::ostringstream stringStream;
		stringStream << "Thread " << threadId;
		std::string threadString = stringStream.str();
		ImGui::Button(threadString.c_str(), { kThreadColumnWidth, height });
		yOffset += height;
	}
	ImGui::EndChild();
}

void ProfilerUI::DrawMarksColumn(const FrameData& aFrameData, float aMarkHeight, float aTotalHeight) const
{
	// Unresizing scrollable parent window
	char name[64];
	char duration[32];
	Utils::StringFormat(name, "%llu##MarksColumn", aFrameData.myFrameNum);
	ImGui::BeginChild(name, { 0, aTotalHeight }, false, ImGuiWindowFlags_HorizontalScrollbar);

	// Zoomable child window
	const float fixedWindowWidth = ImGui::GetWindowWidth();
	float plotWidth = fixedWindowWidth * myWidthScale;
	plotWidth = std::max(plotWidth, fixedWindowWidth);
	Utils::StringFormat(name, "%llu##ZoomWindow", aFrameData.myFrameNum);
	ImGui::BeginChild(name, { plotWidth, aTotalHeight - ImGui::GetStyle().ScrollbarSize });

	// top bar for frame
	const long long frameDuration = aFrameData.myFrameEnd - aFrameData.myFrameStart;
	DurationToString(duration, frameDuration);
	Utils::StringFormat(name, "Duration: %s", duration);
	ImGui::SetCursorPosX(0);
	ImGui::SetNextItemWidth(kThreadColumnWidth);
	ImGui::Button(name, { plotWidth, aMarkHeight });

	const long long frameStart = aFrameData.myFrameStart;
	const long long frameEnd = aFrameData.myFrameEnd;

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

	float yOffset = aMarkHeight;
	for (const auto& threadIdLevelPair : aFrameData.myMaxLevels)
	{
		const std::thread::id threadId = threadIdLevelPair.first;
		const uint32_t maxLevel = threadIdLevelPair.second;
		const float widthPerDurationRatio = plotWidth / frameDuration;
		
		const MarksVec& threadMarks = aFrameData.myThreadMarkMap.at(threadId);
		for (const Mark& mark : threadMarks)
		{
			const float y = yOffset + mark.myDepth * aMarkHeight;
			const float x = plotWidth * InverseLerpProfile(mark.myStart, frameStart, frameEnd);
			const int colorInd = mark.myId % (sizeof(kColors) / sizeof(ImU32));
			DrawMark(mark, {x, y}, plotWidth, aMarkHeight, kColors[colorInd], {frameStart, frameEnd});
			
		}
		yOffset += (maxLevel + 1) * aMarkHeight;
	}
	ImGui::EndChild();
	ImGui::EndChild();
}

void ProfilerUI::DrawMark(const Mark& aMark, glm::vec2 aPos, float aPlotWidth, float aMarkHeight, ImU32 aColor, glm::u64vec2 aFrame) const
{
	const uint64_t frameStart = aFrame.x;
	const uint64_t frameEnd = aFrame.y;

	const long long markDuration = aMark.myEnd - aMark.myStart;
	const long long startTimeOffset = aMark.myStart - frameStart;
	
	float width = aPlotWidth - aPos.x; // by default, max possible

	const bool isMarkFinished = aMark.myEnd != frameEnd;
	if (isMarkFinished)
	{
		// if we have a closed mark, then we can calculate actual width
		const float widthRatio = InverseLerpProfile(aMark.myEnd, frameStart, frameEnd);
		width = widthRatio * aPlotWidth - aPos.x;
	}
	// avoid getting to 0, as ImGUI usess it as a default value
	// to fit the internal label
	width = std::max(width, 1.f);

	ImGui::SetCursorPos({ aPos.x, aPos.y });

	ImGui::PushStyleColor(ImGuiCol_Button, aColor);

	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
	ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_WHITE);

	char name[64];
	char duration[32];
	if (isMarkFinished)
	{
		DurationToString(duration, markDuration);
		Utils::StringFormat(name, "%s - %s", aMark.myName, duration);
	}
	else
	{
		Utils::StringFormat(name, "%s - Ongoing", aMark.myName);
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