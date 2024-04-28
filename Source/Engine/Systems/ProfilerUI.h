#pragma once

class ProfilerUI
{
	constexpr static float kThreadColumnWidth = 80.f;
	constexpr static float kPixelsPerMs = 50.f;
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
	void DrawScopesView();
	void DrawThreadColumn(float aMarkHeight, float aTotalHeight) const;
	void DrawFrames(std::span<FrameData> aData, float& aScale) const;
	void DrawFrameMarks(const FrameData& aFrameData, glm::vec2 aPos, float aMarkHeight, float aFrameWidth) const;
	void DrawMark(const Mark& aMark, glm::vec2 aPos, float aFrameWidth, float aMarkHeight, ImU32 aColor, glm::u64vec2 aFrameTimes) const;
	void UpdateThreadMapping();
	static float CalcMaxZoomOut(std::span<FrameData> aData, float aWidth);

	struct ScopeData
	{
		std::string_view myScopeName;
		size_t myTotalCount = 0;
		size_t myFoundInFrameCount = 0;
		size_t myAvgPerFrameCount = 0;
		uint64_t myMin = std::numeric_limits<uint64_t>::max();
		uint64_t myMax = 0;
		uint64_t myMedian = 0;
		uint64_t myTotalAvg = 0;
	};

	std::vector<FrameData> myInitFrames;
	std::vector<FrameData> myFrames;
	std::vector<ScopeData> myScopeData;
	std::vector<std::string> myScopeNames = { 
		"Game::SubmitRenderables", 
		"Graphics::Gather",
		"GraphicsGL::ExecuteJobs",
		"PhysicsUpdate",
		"RemoveGameObjects",
		"EarlyChecks",
		"FillUBO",
		"BuildRenderJob"
	};
	struct ThreadInfo
	{
		std::thread::id myId;
		uint32_t myMaxLevel = 1;
		uint32_t myAboveMaxLevel = 0;
	};
	std::vector<ThreadInfo> myThreadMapping;
	float myInitFramesScale = 1.f;
	float myFramesScale = 1.f;
	bool myAutoRecordLongFrames = true;
	bool myNeedsToUpdateScopeData = false;
};