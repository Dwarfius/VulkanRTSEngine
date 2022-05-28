#pragma once

class ProfilerUI
{
	constexpr static float kThreadColumnWidth = 80.f;
	constexpr static float kMarkHeight = 30.f;
public:
	struct Mark
	{
		uint64_t myStart;
		uint64_t myEnd;
		char myName[64];
		std::thread::id myThreadId;
		uint32_t myId;
		uint8_t myDepth;

	};
	using MarksVec = std::vector<Mark>;
	using ThreadMarkMap = std::unordered_map<std::thread::id, MarksVec>;

	struct FrameData
	{
		ThreadMarkMap myThreadMarkMap;
		std::unordered_map<std::thread::id, uint32_t> myMaxLevels;
		uint64_t myFrameStart;
		uint64_t myFrameEnd;
		size_t myFrameNum;
	};

public:
	ProfilerUI();
	void Draw(bool& aIsOpen);

private:
	void DrawThreadColumn(const FrameData& aFrameData, float aTotalHeight) const;
	void DrawMarksColumn(const FrameData& aFrameData, float aTotalHeight) const;

	std::vector<FrameData> myFramesToRender;
	float myWidthScale = 1.f;
	bool myAutoRecordLongFrames = true;
};