#include "Precomp.h"
#include "DebugDrawer.h"

#include "Transform.h"

void DebugDrawer::BeginFrame()
{
	UpdateFrameCache();
	UpdateTimedCache();
}

void DebugDrawer::AddTransform(const Transform& aTransform, uint32_t aFramesToLive /*= 1*/)
{
	constexpr float kTransfSize = 0.1f;
	const glm::vec3 center = aTransform.GetPos();
	AddLine(center, center + aTransform.GetRight() * kTransfSize, glm::vec3(1, 0, 0), aFramesToLive);
	AddLine(center, center + aTransform.GetUp() * kTransfSize, glm::vec3(0, 1, 0), aFramesToLive);
	AddLine(center, center + aTransform.GetForward() * kTransfSize, glm::vec3(0, 0, 1), aFramesToLive);
}

void DebugDrawer::AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aColor, uint32_t aFramesToLive /*= 1*/)
{
	AddLine(aFrom, aTo, aColor, aColor, aFramesToLive);
}

void DebugDrawer::AddLine(glm::vec3 aFrom, glm::vec3 aTo, glm::vec3 aFromColor, glm::vec3 aToColor, uint32_t aFramesToLive /*= 1*/)
{
	if (aFramesToLive == 1)
	{
		tbb::spin_mutex::scoped_lock lock(myCacheMutex);
		FrameCache& cache = myFrameCaches.GetWrite();

		if (cache.myVertices.EmplaceBack(aFrom, aFromColor))
		{
			ASSERT_STR(!cache.myVertices.NeedsToGrow(), "Size should be a multiple of 2");
			cache.myVertices.EmplaceBack(aTo, aToColor);
		}
		// For this frame we filled up the cache, so
		// skip it till we grow it next frame
	}
	else
	{
		tbb::spin_mutex::scoped_lock lock(myTimedCacheMutex);
		// do we have space this frame?
		if (myTimedOffsets.PushBack(aFramesToLive))
		{
			PosColorVertex a1 = { aFrom, aFromColor };
			PosColorVertex a2 = { aTo, aToColor };
			myTimedVertices.EmplaceBack(a1, a2);
		}
		else
		{
			ASSERT_STR(false, "There wasn't enough storage for debug timed lines, increase size!");
			return;
		}
	}
}

const PosColorVertex* DebugDrawer::GetCurrentVertices() const
{
	return myFrameCaches.GetRead().myVertices.Data();
}

size_t DebugDrawer::GetCurrentVertexCount() const
{
	return myFrameCaches.GetRead().myVertices.Size();
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
	bool needsToGrow = oldCache.myVertices.NeedsToGrow()
		|| cache.myVertices.Capacity() < oldCache.myVertices.Capacity();
	if (needsToGrow)
	{
		cache.myVertices.Grow();
	}
	cache.myVertices.Clear();
}

void DebugDrawer::UpdateTimedCache()
{
	tbb::spin_mutex::scoped_lock lock(myTimedCacheMutex);
	// the goal is to accumulate all expired ones at the end,
	// so that we have to do less iterations overall and support
	// O(1) insertion complexity
	size_t count = myTimedOffsets.Size();
	for (size_t i = 0; i < count;)
	{
		myTimedOffsets[i]--;
		// has it expired?
		if (myTimedOffsets[i] == 0)
		{
			// yeah - rotate it out to the end if possible
			count--;
			// not checking whether i == count because
			// want to avoid branch, and think doing 2 extra swaps with
			// itself is better
			std::swap(myTimedOffsets[i], myTimedOffsets[count]);
			std::swap(myTimedVertices[i], myTimedVertices[count]);
		}
		else
		{
			// no - add it for drawing
			const TimedLineDrawCmd& cmd = myTimedVertices[i];
			AddLine(cmd.myP1.myPos, cmd.myP2.myPos, cmd.myP1.myColor, cmd.myP2.myColor);
			i++;
		}
	}

	myTimedOffsets.ClearFrom(count);
	myTimedVertices.ClearFrom(count);
}