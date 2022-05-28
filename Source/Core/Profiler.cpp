#include "Precomp.h"
#include "Profiler.h"

Profiler::Storage& Profiler::GetStorage(Profiler& aProfiler)
{
    static thread_local Storage threadStorage(aProfiler);
    return threadStorage;
}

Profiler::Profiler()
{
	myTLSStorages.reserve(std::thread::hardware_concurrency());
	NewFrame();
}

void Profiler::NewFrame()
{
    ScopedMark mark("Profiler::NewFrame", *this);

    Stamp endTime = Clock::now().time_since_epoch().count();

    const uint32_t thisProfileInd = myFrameNum % kMaxFrames;
    const uint32_t nextProfileInd = (myFrameNum + 1) % kMaxFrames;
    std::vector<Mark> allFrameStartMarks;
    std::vector<Mark> allFrameEndMarks;
    // counts of marks-per-frame should be stable in general case, so
    // try to use it to amortise the allocations
    allFrameStartMarks.reserve(myFrameProfiles[nextProfileInd].myStartMarks.size());
    allFrameEndMarks.reserve(myFrameProfiles[nextProfileInd].myStartMarks.size());

    {
        Mark mark;
        while (myStartMarks.try_pop(mark))
        {
            allFrameStartMarks.push_back(mark);
        }
    }

    {
        Mark mark;
        while (myEndMarks.try_pop(mark))
        {
            allFrameEndMarks.push_back(mark);
        }
    }

    FrameProfile& profile = myFrameProfiles[thisProfileInd];
    profile.myEndStamp = endTime;
    profile.myStartMarks = std::move(allFrameStartMarks);
    profile.myEndMarks = std::move(allFrameEndMarks);
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
        int64_t delta = profile.myEndStamp - profile.myBeginStamp;
        using namespace std::chrono_literals;
        constexpr std::chrono::duration kLimit = 16ms;
        if (delta > std::chrono::duration_cast<std::chrono::nanoseconds>(kLimit).count())
        {
            myOnLongFrameCB(profile);
        }
    }
    myFrameNum++;
}

Profiler::Storage::Storage(Profiler& aProfiler)
    : myIdCounter(aProfiler.myIdCounter)
    , myStartMarks(aProfiler.myStartMarks)
    , myEndMarks(aProfiler.myEndMarks)
{
    aProfiler.AddStorage(this);
}

uint32_t Profiler::Storage::StartScope(std::string_view aName)
{
    Mark newMark;
    newMark.myStamp = Clock::now().time_since_epoch().count();
    std::memcpy(newMark.myName, aName.data(), aName.size());
    newMark.myName[aName.size()] = 0;
    newMark.myId = myIdCounter++;
    newMark.myThreadId = std::this_thread::get_id();
    newMark.myDepth = myDepth++;

    myStartMarks.push(newMark);

    return newMark.myId;
}

void Profiler::Storage::EndScope(uint32_t anId, std::string_view aName)
{
    Mark newMark;
    newMark.myStamp = Clock::now().time_since_epoch().count();
    std::memcpy(newMark.myName, aName.data(), aName.size());
    newMark.myName[aName.size()] = 0;
    newMark.myId = anId;
    newMark.myThreadId = std::this_thread::get_id();
    newMark.myDepth = --myDepth;
    
    myEndMarks.push(newMark);
}