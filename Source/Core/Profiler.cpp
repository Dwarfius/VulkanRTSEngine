#include "Precomp.h"
#include "Profiler.h"

Profiler::Storage& Profiler::GetStorage()
{
    static thread_local Storage threadStorage = [] {
        Profiler& profiler = GetInstance();
        //Storage storage(profiler.myIdCounter, profiler);
        //profiler.AddStorage(&threadStorage);
        // Reason why we pass in the profiler is to structure the
        // code in a way to trigger required RVO, to bypass copies of
        // internal mutexes. Thanks C++17!
        return Storage(profiler.myIdCounter, profiler);
    }();
    return threadStorage;
}

Profiler::Profiler()
    : myFrameNum(0)
{
	myTLSStorages.reserve(std::thread::hardware_concurrency());
	NewFrame();
}

void Profiler::NewFrame()
{
	myIdCounter = 0;

    const uint32_t thisProfileInd = myFrameNum % kMaxFrames;
    const uint32_t nextProfileInd = (myFrameNum + 1) % kMaxFrames;
    std::vector<Mark> allFrameMarks;
    // counts of marks-per-frame should be stable in general case, so
    // try to use it to amortise the allocations
    allFrameMarks.reserve(myFrameProfiles[nextProfileInd].myFrameMarks.size());
	for (Storage* storage : myTLSStorages)
	{
        storage->NewFrame(allFrameMarks);
	}
    FrameProfile& profile = myFrameProfiles[thisProfileInd];
    profile.myEndStamp = Clock::now();
    profile.myFrameMarks = std::move(allFrameMarks);
    profile.myFrameNum = myFrameNum;
    myFrameProfiles[nextProfileInd].myBeginStamp = profile.myEndStamp;
    myFrameProfiles[nextProfileInd].myEndStamp = profile.myEndStamp; // current frame, not finished yet

    if (myFrameNum > 0 && myFrameNum < kInitFrames + 1)
    {
        // store a copy of init frame for future inspection
        myInitFrames[myFrameNum - 1] = profile;
    }

    if (myFrameNum > 0)
    {
        std::chrono::duration delta = profile.myEndStamp - profile.myBeginStamp;
        using namespace std::chrono_literals;
        constexpr std::chrono::duration kLimit = 16ms;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(delta) > kLimit)
        {
            myOnLongFrameCB(profile);
        }
    }
    myFrameNum++;
}

Profiler::Storage::Storage(std::atomic<int>& aGlobalCounter, Profiler& aProfiler)
    : myIdCounter(aGlobalCounter)
{
    myMarks.reserve(256);
    aProfiler.AddStorage(this);
}

void Profiler::Storage::BeginMark(std::string_view aName)
{
    Mark newMark;
    newMark.myBeginStamp = Clock::now();
    newMark.myEndStamp = newMark.myBeginStamp; // tag that this mark isn't finished yet
    std::memcpy(newMark.myName, aName.data(), aName.size());
    newMark.myName[aName.size()] = 0;
    newMark.myId = myIdCounter++;
    newMark.myThreadId = std::this_thread::get_id();

    {
        // Because some tasks can be cross-frame, it's possible that
        // a thread will start the mark in the middle of us preparing for
        // a new frame
        tbb::spin_mutex::scoped_lock lock(myStackMutex);

        int parentId = -1;
        if (!myMarkStack.empty())
        {
            parentId = myMarkStack.top().myId;
        }
        newMark.myParentId = parentId;
        myMarkStack.push(newMark);
    }
}

void Profiler::Storage::EndMark()
{
    const Stamp timeStampBeforeLock = Clock::now();

    Mark lastMark;

    {
        // Because some tasks can be cross-frame, it's possible that
        // a thread will end the mark in the middle of us preparing for
        // a new frame
        tbb::spin_mutex::scoped_lock lock(myStackMutex);
        lastMark = myMarkStack.top();
        myMarkStack.pop();
    }
    
    // close up the mark
    lastMark.myEndStamp = timeStampBeforeLock;

    {
        // same as above
        std::lock_guard lock(myMarksMutex);
        myMarks.push_back(lastMark);
    }
}

void Profiler::Storage::NewFrame(std::vector<Mark>& aBuffer)
{
    {
        // Because some tasks can be cross-frame, it's possible that
        // a thread will end the mark in the middle of us preparing for
        // a new frame
        std::lock_guard lock(myMarksMutex);
        aBuffer.insert(aBuffer.end(), myMarks.begin(), myMarks.end());
        myMarks.clear();
    }

    {
        tbb::spin_mutex::scoped_lock lock(myStackMutex);

        // We need to uppend unfinished marks to the buffer, because they are still
        // valid profile marks, and they will be finished during next frame
        constexpr uint32_t unfinishedMarksBufferSize = 32;
        Mark unfinishedMarksBuffer[unfinishedMarksBufferSize];
        ASSERT_STR(myMarkStack.size() < unfinishedMarksBufferSize, "Buffer too small, need an increase!");
        uint32_t bufferInd = 0;
        aBuffer.reserve(aBuffer.size() + myMarkStack.size());
        while (!myMarkStack.empty())
        {
            unfinishedMarksBuffer[bufferInd] = myMarkStack.top();
            aBuffer.push_back(unfinishedMarksBuffer[bufferInd]);
            bufferInd++;
            myMarkStack.pop();
        }

        // Need to reconstruct the stack(with the right order) 
        // so that EndMark calls are successful in the new frame
        for (; bufferInd > 0; bufferInd--)
        {
            uint32_t index = bufferInd - 1;
            myMarkStack.push(unfinishedMarksBuffer[index]);
        }
    }
}