#include "Precomp.h"
#include "Profiler.h"

Profiler::Storage& Profiler::GetStorage()
{
    static thread_local Storage threadStorage = [] {
        Profiler& profiler = GetInstance();
        Storage storage(profiler.myIdCounter);
        profiler.AddStorage(&threadStorage);
        return storage;
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
        storage->AppendMarks(allFrameMarks);
        storage->NewFrame();
	}
    FrameProfile& profile = myFrameProfiles[thisProfileInd];
    profile.myEndStamp = Clock::now();
    profile.myFrameMarks = std::move(allFrameMarks);
    profile.myFrameNum = myFrameNum;
    myFrameProfiles[nextProfileInd].myBeginStamp = profile.myEndStamp;
    myFrameProfiles[nextProfileInd].myEndStamp = profile.myEndStamp; // current frame, not finished yet

    if (myFrameNum == 1)
    {
        // store a copy of init frame for future inspection
        myInitFrame = profile;
    }
    myFrameNum++;
}

void Profiler::Storage::BeginMark(std::string_view aName)
{
    Mark newMark;
    newMark.myBeginStamp = Clock::now();
    std::memcpy(newMark.myName, aName.data(), aName.size());
    newMark.myName[aName.size()] = 0;
    newMark.myId = myIdCounter++;
    // temporarily we're going to store local index of the parent
    // instead of the global ID, it will be replaced in EndMark
    // This is done to simplify parent lookup for Begin-/EndMark
    newMark.myParentId = myActiveMarkInd;
    newMark.myThreadId = std::this_thread::get_id();
    myMarks.push_back(std::move(newMark));
    ASSERT_STR(myMarks.size() < std::numeric_limits<int>::max(),
        "About to pass the supported limit, following static_cast will no longer be valid!");
    myActiveMarkInd = static_cast<int>(myMarks.size()) - 1;
}

void Profiler::Storage::EndMark()
{
    Mark& lastMark = myMarks[myActiveMarkInd];
    // close up the mark
    lastMark.myEndStamp = Clock::now();
    // point at parent for active mark scope
    myActiveMarkInd = lastMark.myParentId;
    lastMark.myParentId = (myActiveMarkInd != -1) ? myMarks[myActiveMarkInd].myId : -1;
}

void Profiler::Storage::AppendMarks(std::vector<Mark>& aBuffer) const
{
    aBuffer.insert(aBuffer.end(), myMarks.begin(), myMarks.end());
}