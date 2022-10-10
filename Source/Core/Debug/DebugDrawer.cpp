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
			myTimedVertices.EmplaceBack(std::move(a1), std::move(a2));
		}
		else
		{
			ASSERT_STR(false, "There wasn't enough storage for debug timed lines, increase size!");
		}
	}
}

void DebugDrawer::AddCircle(glm::vec3 aCenter, glm::vec3 aUp, float aRadius, glm::vec3 aColor, uint32_t aFramesToLive /* = 1*/)
{
	constexpr char kSegments = 16;
	constexpr glm::vec3 kUp{ 0, 1, 0 };
	constexpr glm::vec3 kRight{ 1, 0, 0 };
	constexpr glm::vec3 kForward{ 0, 0, 1 };
	const bool isNormalUp = aUp == kUp || aUp == -kUp;
	const glm::vec3 right = isNormalUp ? kRight : glm::cross(aUp, kUp);
	const glm::vec3 forward = isNormalUp ? kForward : glm::cross(aUp, right);
	float angle = 0;
	constexpr float kDeltaAngle = glm::two_pi<float>() / kSegments;
	for (char i = 0; i < kSegments; i++)
	{
		const float x1 = glm::cos(angle);
		const float y1 = glm::sin(angle);
		const glm::vec3 oldDir = glm::normalize(x1 * right + y1 * forward);

		angle += kDeltaAngle;

		const float x2 = glm::cos(angle);
		const float y2 = glm::sin(angle);
		const glm::vec3 newDir = glm::normalize(x2 * right + y2 * forward);

		const glm::vec3 from = aCenter + oldDir * aRadius;
		const glm::vec3 to = aCenter + newDir * aRadius;
		AddLine(from, to, aColor, aFramesToLive);
	}
}

void DebugDrawer::AddSphere(glm::vec3 aCenter, float aRadius, glm::vec3 aColor, uint32_t aFramesToLive /* = 1 */)
{
	AddCircle(aCenter, glm::vec3{ 1, 0, 0 }, aRadius, aColor, aFramesToLive);
	AddCircle(aCenter, glm::vec3{ 0, 1, 0 }, aRadius, aColor, aFramesToLive);
	AddCircle(aCenter, glm::vec3{ 0, 0, 1 }, aRadius, aColor, aFramesToLive);
}

void DebugDrawer::AddAABB(glm::vec3 aMin, glm::vec3 aMax, glm::vec3 aColor, uint32_t aFramesToLive /* = 1 */)
{
	// bottom square
	const glm::vec3 aBot = aMin;
	const glm::vec3 bBot{ aMax.x, aMin.y, aMin.z };
	const glm::vec3 cBot{ aMax.x, aMin.y, aMax.z };
	const glm::vec3 dBot{ aMin.x, aMin.y, aMax.z };
	AddLine(aBot, bBot, aColor);
	AddLine(aBot, dBot, aColor);
	AddLine(bBot, cBot, aColor);
	AddLine(dBot, cBot, aColor);

	// top square
	const glm::vec3 aTop{ aBot.x, aMax.y, aBot.z };
	const glm::vec3 bTop{ bBot.x, aMax.y, bBot.z };
	const glm::vec3 cTop{ cBot.x, aMax.y, cBot.z };
	const glm::vec3 dTop{ dBot.x, aMax.y, dBot.z };
	AddLine(aTop, bTop, aColor);
	AddLine(aTop, dTop, aColor);
	AddLine(bTop, cTop, aColor);
	AddLine(dTop, cTop, aColor);

	// verticals
	AddLine(aBot, aTop, aColor);
	AddLine(bBot, bTop, aColor);
	AddLine(cBot, cTop, aColor);
	AddLine(dBot, dTop, aColor);
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