#include "Precomp.h"
#include "Profiler.h"

#include "LazyVector.h"

class AtomicLazyMarksVector
{
public:
    Profiler::Mark* Allocate()
    {
        std::lock_guard lock(myMutex);
        if (myMarks.EmplaceBack()) [[likely]]
        {
            return &myMarks[myMarks.Size() - 1];
        }
        return nullptr;
    }

    void Transfer(std::vector<Profiler::Mark>& aMarks)
    {
        std::lock_guard lock(myMutex);
        aMarks.insert(aMarks.end(), myMarks.begin(), myMarks.end());

        if (myMarks.NeedsToGrow())
        {
            myMarks.Grow();
        }
        else
        {
            myMarks.Clear();
        }
    }
private:
    LazyVector<Profiler::Mark, 256> myMarks;
    std::mutex myMutex;
};

class Profiler::Storage
{
public:
    Storage(Profiler& aProfiler);

    uint32_t StartScope(std::string_view aName);
    void EndScope(uint32_t anId, std::string_view aName);
    void NewFrame(std::vector<Mark>& aStartMarks, std::vector<Mark>& aEndMarks);
private:
    std::atomic<uint32_t>& myIdCounter;
    AtomicLazyMarksVector myStartMarks;
    AtomicLazyMarksVector myEndMarks;
    uint8_t myDepth = 0;
};

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

    for (Storage* storage : myTLSStorages)
    {
        storage->NewFrame(allFrameStartMarks, allFrameEndMarks);
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
{
    aProfiler.AddStorage(this);
}

uint32_t Profiler::Storage::StartScope(std::string_view aName)
{
    const Stamp startStamp = Clock::now().time_since_epoch().count();

    Mark* newMark = myStartMarks.Allocate();
    if (newMark)
    {
        newMark->myStamp = startStamp;
        std::memcpy(newMark->myName, aName.data(), aName.size());
        newMark->myName[aName.size()] = 0;
        newMark->myId = myIdCounter++;
        newMark->myThreadId = std::this_thread::get_id();
        newMark->myDepth = myDepth++;
        return newMark->myId;
    }

    return 0;
}

void Profiler::Storage::EndScope(uint32_t anId, std::string_view aName)
{
    Mark* newMark = myEndMarks.Allocate();
    if(newMark) [[likely]]
    {
        std::memcpy(newMark->myName, aName.data(), aName.size());
        newMark->myName[aName.size()] = 0;
        newMark->myId = anId;
        newMark->myThreadId = std::this_thread::get_id();
        newMark->myDepth = --myDepth;
        newMark->myStamp = Clock::now().time_since_epoch().count();
    }
}

void Profiler::Storage::NewFrame(std::vector<Mark>& aStartMarks, std::vector<Mark>& aEndMarks)
{
    myStartMarks.Transfer(aStartMarks);
    myEndMarks.Transfer(aEndMarks);
}

Profiler::ScopedMark::ScopedMark(std::string_view aName, Profiler& aProfiler)
    : myProfiler(aProfiler)
    , myName(aName)
{
    myId = GetStorage(myProfiler).StartScope(aName);
}

Profiler::ScopedMark::~ScopedMark()
{
    GetStorage(myProfiler).EndScope(myId, myName);
}