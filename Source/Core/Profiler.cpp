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
        // Note: LazyVector::TransferTo turned out to be a pessimization,
        // because the "heavy" thread can migrate, causing all storages
        // to grow big with low occupancy (5 marks vs 16k storage). See
        // TODO inside LazyVector::TransferTo for more info
        static_assert(std::is_trivially_copyable_v<Profiler::Mark>, "Can't get cheap memcpy!");
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
    void NewFrame(ThreadProfile& aProfile);

private:
    std::atomic<uint32_t>& myIdCounter;
    AtomicLazyMarksVector myStartMarks;
    AtomicLazyMarksVector myEndMarks;
    std::thread::id myThreadId;
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

    FrameProfile& profile = myFrameProfiles[thisProfileInd];
    profile.myEndStamp = endTime;
    profile.myFrameNum = myFrameNum;
    profile.myThreadProfiles.clear();
    profile.myThreadProfiles.reserve(myTLSStorages.size());
    for (Storage* storage : myTLSStorages)
    {
        ThreadProfile threadProfile;
        storage->NewFrame(threadProfile);
        profile.myThreadProfiles.emplace_back(std::move(threadProfile));
    }
    myFrameProfiles[nextProfileInd].myBeginStamp = profile.myEndStamp;
    myFrameProfiles[nextProfileInd].myEndStamp = profile.myEndStamp; // current frame, not finished yet

    if (myFrameNum > 0 && myFrameNum < kInitFrames + 1)
    {
        // store a copy of init frame for future inspection
        myInitFrames[myFrameNum - 1] = profile;
    }

    if ((myFrameReportingEnabled || myCaptureFrame) && myFrameNum > 0)
    {
        int64_t delta = profile.myEndStamp - profile.myBeginStamp;
        using namespace std::chrono_literals;
        constexpr std::chrono::duration kLimit = 16ms;
        if (delta > std::chrono::duration_cast<std::chrono::nanoseconds>(kLimit).count()
            || myCaptureFrame)
        {
            myOnLongFrameCB(profile);
        }
    }
    myFrameNum++;
    myCaptureFrame = false;
}

Profiler::Storage::Storage(Profiler& aProfiler)
    : myIdCounter(aProfiler.myIdCounter)
    , myThreadId(std::this_thread::get_id())
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
        newMark->myDepth = --myDepth;
        newMark->myStamp = Clock::now().time_since_epoch().count();
    }
}

void Profiler::Storage::NewFrame(ThreadProfile& aProfile)
{
    aProfile.myThreadId = myThreadId;
    myStartMarks.Transfer(aProfile.myStartMarks);
    myEndMarks.Transfer(aProfile.myEndMarks);
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