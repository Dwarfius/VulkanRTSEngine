#pragma once

// A singleton class that manages the collection of profiling data in
// profiling Marks, which contains timestamps of start-end, name and thread ids
class Profiler
{
	using Clock = std::chrono::high_resolution_clock;
	using Stamp = Clock::time_point;
    constexpr static uint32_t kMaxFrames = 10;
public:
    class ScopedMark;

	// A profiling mark, a record of how long it took to execute a named activity
    // Also keeps track of it's parent via IDs
    struct Mark
	{
		Stamp myBeginStamp;
		Stamp myEndStamp;
		char myName[64];
		int myId;
		int myParentId;
		std::thread::id myThreadId;
	};

    // A record of a frame's profiling data - how long the entire frame took,
    // what's the frame counter, and all of the marks of that frame
    struct FrameProfile
    {
        Stamp myBeginStamp;
        Stamp myEndStamp;
        size_t myFrameNum;
        std::vector<Mark> myFrameMarks;
    };

    static Profiler& GetInstance()
    {
        static Profiler profiler;
        return profiler;
    }

    // Call to start tracking new Marks for new frame
    void NewFrame();

    const std::array<FrameProfile, kMaxFrames>& GetBufferedFrameData() const { return myFrameProfiles;  }

private:
    class Storage;
    // Returns a thread-local storage for accumulating Marks for current frame
    static Storage& GetStorage();

    // Initializes and automatically prepares for recording into the new frame
    Profiler();

    void AddStorage(Storage* aStorage)
    {
        // we use a spin lock because we try to avoid allocations
        // by reserving enough to support hardware_concurrency.
        // it's not guaranteed, but should be enough for a start
        tbb::spin_mutex::scoped_lock lock(myStorageMutex);
        myTLSStorages.push_back(aStorage);
    }
    std::atomic<int> myIdCounter;
    std::vector<Storage*> myTLSStorages;
    std::array<FrameProfile, kMaxFrames> myFrameProfiles;
    size_t myFrameNum;
    tbb::spin_mutex myStorageMutex;
};

class Profiler::Storage
{
public:
    Storage(std::atomic<int>& aGlobalCounter)
        : myIdCounter(aGlobalCounter)
    {
        myMarks.reserve(256);
        NewFrame();
    }

    void BeginMark(std::string_view aName);
    void EndMark();
    void NewFrame()
    {
        myActiveMarkInd = -1;
        myMarks.clear();
    }
    void AppendMarks(std::vector<Mark>& aBuffer) const;
private:
    std::vector<Mark> myMarks;
    std::atomic<int>& myIdCounter;
    int myActiveMarkInd;
};

// RAII style profiling mark - starts a mark on ctor, stops it on dtor
class Profiler::ScopedMark
{
public:
    ScopedMark(std::string_view aName)
    {
        GetStorage().BeginMark(aName);
    }

    ~ScopedMark()
    {
        GetStorage().EndMark();
    }
};