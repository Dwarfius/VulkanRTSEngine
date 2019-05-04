#include "Precomp.h"
#include "DebugDrawer.h"

DebugDrawer::DebugDrawer()
{
	for (FrameCache& cache : myFrameCaches)
	{
		cache.myVertices.resize(SingleFrameCacheSize);
		cache.myCurrentIndex = 0;
	}

	myTimedOffsets.resize(TimedCacheSize);
	myTimedVertices.resize(TimedCacheSize);
	myFreeOffset = 0;
}

void DebugDrawer::BeginFrame()
{
	UpdateFrameCache();
	UpdateTimedCache();
}

void DebugDrawer::AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor)
{
	AddLine(aFrom, aTo, aColor, aColor);
}

void DebugDrawer::AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aFromColor, glm::vec3 aToColor)
{
	tbb::spin_mutex::scoped_lock lock(myCacheMutex);
	FrameCache& cache = myFrameCaches.GetWrite();
	if (cache.myCurrentIndex == cache.myVertices.size())
	{
		// For this frame we filled up the cache, so
		// skip it till we grow it next frame
		return;
	}

	PosColorVertex& v1 = cache.myVertices[cache.myCurrentIndex++];
	v1.myPos = aFrom;
	v1.myColor = aFromColor;
	PosColorVertex& v2 = cache.myVertices[cache.myCurrentIndex++];
	v2.myPos = aTo;
	v2.myColor = aToColor;
}

void DebugDrawer::AddLineWithLife(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor, uint32_t aFramesToLive)
{
	AddLineWithLife(aFrom, aTo, aColor, aColor, aFramesToLive);
}

void DebugDrawer::AddLineWithLife(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aFromColor, glm::vec3 aToColor, uint32_t aFramesToLive)
{
	tbb::spin_mutex::scoped_lock lock(myTimedCacheMutex);
	// do we have space this frame?
	if (myFreeOffset == myTimedOffsets.size())
	{
		ASSERT_STR(false, "There wasn't enough storage for debug timed lines, increase size!");
		return;
	}

	// we give an extra frame to the life timer in order to make it threadsafe
	// for frame buffering
	myTimedOffsets[myFreeOffset] = aFramesToLive;
	myTimedVertices[myFreeOffset++] = { { aFrom, aFromColor }, { aTo, aToColor } };
}

const PosColorVertex* DebugDrawer::GetCurrentVertices() const
{
	return myFrameCaches.GetRead().myVertices.data();
}

size_t DebugDrawer::GetCurrentVertexCount() const
{
	return myFrameCaches.GetRead().myCurrentIndex;
}

void DebugDrawer::UpdateFrameCache()
{
	// This abuses the spin_mutex, since it might relocate
	// But, this should be a rare occurance, so it's still valid to use it
	tbb::spin_mutex::scoped_lock lock(myCacheMutex);
	myFrameCaches.Advance();

	const FrameCache& oldCache = myFrameCaches.GetRead();
	FrameCache& cache = myFrameCaches.GetWrite();
	// So, either the cache that just switched out wasn't enough to store
	// the data, or the cache that we just switched to is smaller than the
	// old one - which means it wouldn't be able to fit old data, so it needs to grow
	bool needsToGrow = oldCache.myCurrentIndex == oldCache.myVertices.size()
		|| cache.myVertices.size() < oldCache.myVertices.size();
	if (needsToGrow)
	{
		size_t oldSize = cache.myVertices.size();
		cache.myVertices.resize(oldSize << 1);
	}
	cache.myCurrentIndex = 0;
}

void DebugDrawer::UpdateTimedCache()
{
	tbb::spin_mutex::scoped_lock lock(myTimedCacheMutex);
	// the goal is to accumulate all expired ones at the end,
	// so that we have to do less iterations overall and support
	// O(1) insertion complexity
	for (size_t i = 0; i < myFreeOffset;)
	{
		myTimedOffsets[i]--;
		// has it expired?
		if (myTimedOffsets[i] == 0)
		{
			// yeah - rotate it out to the end if possible
			myFreeOffset--;
			// not checking whether i == myFreeOffset because
			// want to avoid branch, and thing doing 2 extra swaps with
			// itself is better
			std::swap(myTimedOffsets[i], myTimedOffsets[myFreeOffset]);
			std::swap(myTimedVertices[i], myTimedVertices[myFreeOffset]);
		}
		else
		{
			// no - add it for drawing
			const TimedLineDrawCmd& cmd = myTimedVertices[i];
			AddLine(cmd.myP1.myPos, cmd.myP2.myPos, cmd.myP1.myColor, cmd.myP2.myColor);
			i++;
		}
	}
}