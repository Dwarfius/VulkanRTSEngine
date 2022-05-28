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
			mark.myStart = startMark.myStamp.time_since_epoch().count();
			mark.myEnd = aProfile.myEndStamp.time_since_epoch().count();
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
				iter->second.myEnd = endMark.myStamp.time_since_epoch().count();
			}
			else
			{
				ProfilerUI::Mark mark;
				mark.myId = endMark.myId;
				mark.myThreadId = endMark.myThreadId;
				mark.myStart = aProfile.myBeginStamp.time_since_epoch().count();
				mark.myEnd = endMark.myStamp.time_since_epoch().count();
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
		data.myFrameStart = aProfile.myBeginStamp.time_since_epoch().count();
		data.myFrameEnd = aProfile.myEndStamp.time_since_epoch().count();
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
			Utils::StringFormat(nodeName, "Frame %llu", frameData.myFrameNum);
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
	Utils::StringFormat(name, "%llu##ThreadColumn", aFrameData.myFrameNum);
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
	ImGui::Button(name, { plotWidth, kMarkHeight });

	const long long frameStart = aFrameData.myFrameStart;
	const long long frameEnd = aFrameData.myFrameEnd;

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
		for (const Mark& mark : threadMarks)
		{
			const long long markDuration = mark.myEnd - mark.myStart;
			const long long startTimeOffset = mark.myStart - frameStart;

			const float y = yOffset + mark.myDepth * kMarkHeight;
			const float x = plotWidth * InverseLerpProfile(mark.myStart);
			float width = plotWidth - x; // by default, max possible

			const bool isMarkFinished = mark.myEnd != frameEnd;
			if (isMarkFinished)
			{
				// if we have a closed mark, then we can calculate actual width
				const float widthRatio = InverseLerpProfile(mark.myEnd);
				width = widthRatio * plotWidth - x;
			}
			// avoid getting to 0, as ImGUI usess it as a default value
			// to fit the internal label
			width = std::max(width, 1.f);

			ImGui::SetCursorPos({ x, y });

			const int colorInd = mark.myId % (sizeof(kColors) / sizeof(ImU32));
			ImGui::PushStyleColor(ImGuiCol_Button, kColors[colorInd]);

			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1);
			ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32_WHITE);

			if (isMarkFinished)
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
				if (isMarkFinished)
				{
					ImGui::SetTooltip("Name: %s\nDuration: %s",
						mark.myName, duration);
				}
				else
				{
					ImGui::SetTooltip("Name: %s\nDuration: Ongoing",
						mark.myName);
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