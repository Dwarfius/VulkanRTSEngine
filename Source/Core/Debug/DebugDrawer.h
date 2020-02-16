#pragma once

#include "../RWBuffer.h"
#include "../Vertex.h"
#include "../LazyVector.h"

// TODO: add line life support
// A threadsafe debug drawing utility class. 
// Has capacity for 2 frames - current and past.
class DebugDrawer
{
public:
	// Call this at the earliest opportunity in the frame to
	// prepare accumulating debug lines for new frame
	void BeginFrame();
	
	// Creates a line from aFrom to aTo with a single color
	void AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor);
	// Creates a line from aFrom and color aFromColor to aTo and color aToColor
	void AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aFromColor, glm::vec3 aToColor);

	// Creates a line like usual, except it will persist for multiple frames
	void AddLineWithLife(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor, uint32_t aFramesToLive);
	// Creates a line like usual, except it will persist for multiple frames
	void AddLineWithLife(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aFromColor, glm::vec3 aToColor, uint32_t aFramesToLive);

	const PosColorVertex* GetCurrentVertices() const;
	size_t GetCurrentVertexCount() const;
	int GetVertexType() const { return PosColorVertex::Type; }

private:
	constexpr static size_t SingleFrameCacheSize = 1 << 10;
	struct FrameCache
	{
		LazyVector<PosColorVertex, SingleFrameCacheSize> myVertices;
	};
	RWBuffer<FrameCache, 2> myFrameCaches;
	tbb::spin_mutex myCacheMutex;

	void UpdateFrameCache();

	constexpr static size_t TimedCacheSize = 1 << 10;
	struct TimedLineDrawCmd
	{
		PosColorVertex myP1, myP2;

		TimedLineDrawCmd() = default;
		TimedLineDrawCmd(PosColorVertex aP1, PosColorVertex aP2)
			: myP1(aP1)
			, myP2(aP2)
		{
		}
	};
	// separate vectors to better utilize cache during iteration
	LazyVector<uint32_t, TimedCacheSize> myTimedOffsets;
	LazyVector<TimedLineDrawCmd, TimedCacheSize> myTimedVertices;
	tbb::spin_mutex myTimedCacheMutex;

	void UpdateTimedCache();
};